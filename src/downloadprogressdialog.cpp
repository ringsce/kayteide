#include "downloadprogressdialog.h"
#include "ui_downloadprogressdialog.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>

// Constructor and Destructor
DownloadProgressDialog::DownloadProgressDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DownloadProgressDialog)
    , process(new QProcess(this))
    , currentRepoIndex(0)
    , currentSystemToolCheckIndex(0)
    , currentMissingToolInstallIndex(0)
    , homebrewIsInstalled(false)
    , currentStage(InitialStage)
{
    ui->setupUi(this);
    setModal(true);

    setupProcessEnvironment(); // Do this immediately after process is created.

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DownloadProgressDialog::handleCancelButtonClicked);

    connect(process, &QProcess::started, this, &DownloadProgressDialog::handleProcessStarted);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DownloadProgressDialog::handleProcessFinished);
    connect(process, &QProcess::readyReadStandardOutput,
            this, &DownloadProgressDialog::handleProcessReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError,
            this, &DownloadProgressDialog::handleProcessReadyReadStandardError);
    connect(process, &QProcess::errorOccurred,
            this, &DownloadProgressDialog::handleProcessErrorOccurred);

    systemToolsRequired << "git" << "cmake" << "make";

    setStatus(tr("Initializing..."));
    log(tr("DownloadProgressDialog initialized."));
}

DownloadProgressDialog::~DownloadProgressDialog()
{
    delete ui;
}

// Environment Setup
void DownloadProgressDialog::setupProcessEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", "/opt/homebrew/bin:/usr/local/bin:" + env.value("PATH"));
    process->setProcessEnvironment(env);
    qDebug() << "Process PATH set to:" << env.value("PATH");
}

// Main Process Initiation
void DownloadProgressDialog::startProcess(const QStringList &repos, const QString &downloadDir)
{
    reposToDownload.clear();
    for (const QString& repoStr : repos) {
        QStringList parts = repoStr.split(';');
        if (parts.size() >= 2) {
            GitHubRepo repo;
            repo.url = parts.at(0);
            repo.localDirName = parts.at(1);
            if (parts.size() >= 3) {
                repo.branch = parts.at(2);
            }
            reposToDownload.append(repo);
        }
    }
    this->downloadDestinationDir = downloadDir;

    ui->progressBarOverall->setRange(0, reposToDownload.size());
    ui->progressBarOverall->setValue(0);
    currentRepoIndex = 0;

    ui->repoListWidget->clear();

    setStatus(tr("Checking system prerequisites..."));
    log(tr("Starting system tool checks..."));

    startXcodeToolsCheck(); // Begin the first stage
}

// --- General Process Handlers ---
void DownloadProgressDialog::handleProcessStarted()
{
    log(tr("Process started for stage: %1").arg(currentStage));
}

void DownloadProgressDialog::handleProcessReadyReadStandardOutput()
{
    QByteArray data = process->readAllStandardOutput();
    // Decide whether to log this verbosely or only for specific stages
    // For now, let's log everything to debug
    log(QString::fromUtf8(data));
}

void DownloadProgressDialog::handleProcessReadyReadStandardError()
{
    QByteArray data = process->readAllStandardError();
    log(tr("ERROR (stderr): %1").arg(QString::fromUtf8(data)));
}

void DownloadProgressDialog::handleProcessErrorOccurred(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart: errorString = "Failed to start"; break;
        case QProcess::Crashed: errorString = "Crashed"; break;
        case QProcess::Timedout: errorString = "Timed out"; break;
        case QProcess::ReadError: errorString = "Read error"; break;
        case QProcess::WriteError: errorString = "Write error"; break;
        case QProcess::UnknownError: errorString = "Unknown error"; break;
        default: errorString = "No error"; break;
    }
    log(tr("Process error occurred: %1 - %2").arg(errorString).arg(process->errorString()));
    if (currentStage != BrewInstallStage) { // Brew install might often need user input or permissions
        QMessageBox::critical(this, tr("Process Error"),
                              tr("An error occurred during process execution: %1.\nCheck log for details.").arg(errorString));
        emit processAborted();
        reject();
    }
}

void DownloadProgressDialog::handleCancelButtonClicked()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished(1000); // Give it a moment to terminate
    }
    emit processAborted();
    reject(); // Close dialog
}

// --- Stage-specific Handlers and Flow Control ---

// Defines the next action after a single tool check completes
void DownloadProgressDialog::checkNextTool()
{
    if (currentSystemToolCheckIndex < systemToolsRequired.size()) {
        QString tool = systemToolsRequired.at(currentSystemToolCheckIndex);
        setStatus(tr("Checking for tool: %1 (via Homebrew)...").arg(tool));
        log(tr("Running: brew list %1").arg(tool));

        process->start("brew", QStringList() << "list" << tool);
    } else {
        // All individual tools checked
        setStatus(tr("All system tool checks complete."));
        log(tr("\n--- System Tool Check Summary ---"));
        log(tr("Found tools: %1").arg(foundTools.join(", ")));
        log(tr("Missing tools: %1").arg(missingTools.join(", ")));
        log(tr("---------------------------------\n"));

        if (!missingTools.isEmpty()) {
            startToolInstall(); // Initiate installation for missing tools
        } else {
            finishAllChecksAndStartDownloads(); // All tools are present, proceed
        }
    }
}

// Defines the next action after a single tool installation attempt
void DownloadProgressDialog::installNextMissingTool()
{
    if (currentMissingToolInstallIndex < missingTools.size()) {
        QString toolToInstall = missingTools.at(currentMissingToolInstallIndex);
        setStatus(tr("Installing missing tool: %1").arg(toolToInstall));
        log(tr("Running: brew install %1").arg(toolToInstall));

        int originalCheckIndex = systemToolsRequired.indexOf(toolToInstall);
        if (originalCheckIndex != -1 && originalCheckIndex < ui->repoListWidget->count()) {
             ui->repoListWidget->item(originalCheckIndex)->setText(tr("Installing: %1...").arg(toolToInstall));
        } else {
             ui->repoListWidget->addItem(tr("Installing: %1...").arg(toolToInstall));
        }

        process->start("brew", QStringList() << "install" << toolToInstall);
    } else {
        // All missing tools have been attempted for installation
        log(tr("\n--- Missing Tool Installation Attempt Complete ---"));
        // After installation attempts, we proceed. Final check on tool presence could be added here.
        finishAllChecksAndStartDownloads();
    }
}

// Handles completion of a single repository download
void DownloadProgressDialog::handleSingleRepoDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    log(tr("Download process finished with exit code %1, status %2 for repo %3.")
        .arg(exitCode).arg(exitStatus).arg(reposToDownload.at(currentRepoIndex).localDirName));

    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        log(tr("Successfully downloaded: %1").arg(reposToDownload.at(currentRepoIndex).localDirName));
        ui->repoListWidget->addItem(tr("✔ Downloaded: %1").arg(reposToDownload.at(currentRepoIndex).localDirName));
    } else {
        log(tr("Failed to download: %1").arg(reposToDownload.at(currentRepoIndex).localDirName));
        ui->repoListWidget->addItem(tr("✘ Failed: %1").arg(reposToDownload.at(currentRepoIndex).localDirName));
        QMessageBox::warning(this, tr("Download Failed"),
                             tr("Failed to download %1.\nCheck log for details.")
                             .arg(reposToDownload.at(currentRepoIndex).localDirName));
    }

    currentRepoIndex++;
    updateOverallProgress();
    processNextRepo(); // Continue to the next repo or finish
}


// Unified handler for QProcess::finished (orchestrates stages)
void DownloadProgressDialog::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Process finished for stage:" << currentStage << " Exit code:" << exitCode << " Status:" << exitStatus;
    switch (currentStage) {
        case XcodeToolsCheckStage:
            handleXcodeToolsCheckFinished(exitCode, exitStatus);
            break;
        case BrewCheckStage:
            handleBrewCheckFinished(exitCode, exitStatus);
            break;
        case BrewInstallStage:
            handleBrewInstallFinished(exitCode, exitStatus);
            break;
        case ToolCheckStage:
            handleToolCheckFinished(exitCode, exitStatus);
            break;
        case ToolInstallStage:
            handleToolInstallFinished(exitCode, exitStatus);
            break;
        case DownloadStage:
            handleSingleRepoDownloadFinished(exitCode, exitStatus);
            break;
        case InitialStage: // Should not happen
            log(tr("ERROR: Process finished in InitialStage."));
            processAborted();
            break;
    }
}

void DownloadProgressDialog::startXcodeToolsCheck()
{
    currentStage = XcodeToolsCheckStage;
    setStatus(tr("Checking for Xcode Command Line Tools..."));
    log(tr("Running: xcode-select -p"));
    process->start("xcode-select", QStringList() << "-p");
}

void DownloadProgressDialog::handleXcodeToolsCheckFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    QString output = process->readAllStandardOutput().trimmed();
    // Check for non-empty output and a typical path, or just exitCode == 0 for simple check
    if (exitCode == 0 && !output.isEmpty()) {
        log(tr("Xcode Command Line Tools found at: %1").arg(output));
        startBrewCheck();
    } else {
        log(tr("Xcode Command Line Tools not found or not correctly configured. Output: %1").arg(output));
        QMessageBox::information(this, tr("Install Xcode Command Line Tools"),
                                 tr("Xcode Command Line Tools are required for development. "
                                    "Please install them by running:\n\n"
                                    "xcode-select --install\n\n"
                                    "After installation, please close and reopen the IDE to retry."));
        emit processAborted();
        reject();
    }
}

void DownloadProgressDialog::startBrewCheck()
{
    currentStage = BrewCheckStage;
    setStatus(tr("Checking for Homebrew..."));
    log(tr("Running: which brew"));
    process->start("which", QStringList() << "brew");
}

void DownloadProgressDialog::handleBrewCheckFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    if (exitCode == 0) {
        log(tr("Homebrew is installed."));
        homebrewIsInstalled = true;
        setupProcessEnvironment(); // Ensure PATH is updated if brew was just found but not installed by us
        startToolChecks();
    } else {
        log(tr("Homebrew is NOT installed."));
        homebrewIsInstalled = false;
        startBrewInstall();
    }
}

void DownloadProgressDialog::startBrewInstall()
{
    currentStage = BrewInstallStage;
    setStatus(tr("Installing Homebrew... This may take a while."));
    log(tr("Beginning Homebrew installation. You may be prompted for your password."));

    QString installScript = "curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh";
    process->start("/bin/bash", QStringList() << "-c" << installScript);
}

void DownloadProgressDialog::handleBrewInstallFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    if (exitCode == 0) {
        log(tr("Homebrew installation completed successfully."));
        homebrewIsInstalled = true;
        setupProcessEnvironment(); // Important: Re-setup PATH to ensure brew commands are found
        startToolChecks();
    } else {
        log(tr("Homebrew installation failed. Exit code: %1, Status: %2").arg(exitCode).arg(exitStatus));
        QMessageBox::critical(this, tr("Homebrew Installation Failed"),
                              tr("Homebrew could not be installed. Please install it manually from brew.sh and restart the IDE."));
        emit processAborted();
        reject();
    }
}

void DownloadProgressDialog::startToolChecks()
{
    currentStage = ToolCheckStage;
    currentSystemToolCheckIndex = 0;
    foundTools.clear();
    missingTools.clear();
    ui->repoListWidget->clear();
    for (const QString& tool : systemToolsRequired) {
        ui->repoListWidget->addItem(QString("Checking for: %1...").arg(tool));
    }
    checkNextTool(); // Start the first check
}

void DownloadProgressDialog::handleToolCheckFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    QString tool = systemToolsRequired.at(currentSystemToolCheckIndex);
    QListWidgetItem* currentItem = ui->repoListWidget->item(currentSystemToolCheckIndex);

    if (exitCode == 0) {
        foundTools.append(tool);
        if (currentItem) currentItem->setText(tr("✔ Found: %1").arg(tool));
        log(tr("%1 is installed.").arg(tool));
    } else {
        missingTools.append(tool);
        if (currentItem) currentItem->setText(tr("✘ Missing: %1").arg(tool));
        log(tr("%1 is NOT installed or brew encountered an error.").arg(tool));
    }
    currentSystemToolCheckIndex++;
    checkNextTool(); // Check the next tool
}

void DownloadProgressDialog::startToolInstall()
{
    currentStage = ToolInstallStage;
    currentMissingToolInstallIndex = 0;
    installNextMissingTool(); // Start installation for the first missing tool
}

void DownloadProgressDialog::handleToolInstallFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);
    QString tool = missingTools.at(currentMissingToolInstallIndex);

    int originalCheckIndex = systemToolsRequired.indexOf(tool);
    QListWidgetItem* itemToUpdate = nullptr;
    if (originalCheckIndex != -1 && originalCheckIndex < ui->repoListWidget->count()) {
        itemToUpdate = ui->repoListWidget->item(originalCheckIndex);
    }

    if (exitCode == 0) {
        log(tr("Successfully installed: %1").arg(tool));
        if (itemToUpdate) itemToUpdate->setText(tr("✔ Installed: %1").arg(tool));
    } else {
        log(tr("Failed to install: %1. Exit code: %2").arg(tool).arg(exitCode));
        if (itemToUpdate) itemToUpdate->setText(tr("✘ Failed Install: %1").arg(tool));
        QMessageBox::warning(this, tr("Tool Installation Failed"),
                             tr("Failed to install %1. You may need to install it manually and restart the IDE.").arg(tool));
    }

    currentMissingToolInstallIndex++;
    installNextMissingTool(); // Try to install the next missing tool
}

void DownloadProgressDialog::finishAllChecksAndStartDownloads()
{
    log(tr("All system prerequisites met. Proceeding to downloads."));
    currentStage = DownloadStage;
    processNextRepo(); // Start the actual downloads
}

// --- Repository Download Logic ---
void DownloadProgressDialog::processNextRepo()
{
    if (currentRepoIndex < reposToDownload.size()) {
        GitHubRepo repo = reposToDownload.at(currentRepoIndex);
        // Ensure the base download directory exists
        QDir().mkpath(downloadDestinationDir);

        // Construct the full path for the repository clone
        QString repoPath = downloadDestinationDir + QDir::separator() + repo.localDirName;
        QDir repoDir(repoPath);

        // Check if the directory already exists and is not empty.
        // If it exists and contains files, assume it's already cloned.
        if (repoDir.exists() && !repoDir.entryList(QDir::NoDotAndDotDot|QDir::AllEntries).isEmpty()) {
            log(tr("Repository '%1' already exists and is not empty. Skipping clone.").arg(repo.localDirName));
            ui->repoListWidget->addItem(tr("✔ Already Exists: %1").arg(repo.localDirName));
            currentRepoIndex++; // Move to the next repo
            updateOverallProgress();
            processNextRepo(); // Process next one without running git
            return; // Exit this function call
        }

        // Prepare git clone command
        QStringList arguments;
        arguments << "clone";
        if (!repo.branch.isEmpty()) {
            arguments << "--branch" << repo.branch;
        }
        arguments << repo.url << repoPath;

        setStatus(tr("Cloning %1...").arg(repo.localDirName));
        log(tr("Running: git %1").arg(arguments.join(" ")));

        // Set the working directory for git clone to the download destination parent
        // This makes git clone 'repo.url repoPath' behave correctly if repoPath is relative
        // But since repoPath is absolute, setting workingDirectory is less critical but good practice
        process->setWorkingDirectory(downloadDestinationDir);
        process->start("git", arguments);
    } else {
        // All repositories processed
        setStatus(tr("All downloads complete."));
        log(tr("All repositories have been processed."));
        emit processCompleted(); // Signal overall success
        accept(); // Close dialog as successful
    }
}

// --- UI Update Helpers ---
void DownloadProgressDialog::log(const QString &message)
{
    // Append message to the text browser (now QTextEdit), ensure scroll to bottom
    ui->logTextEdit->append(message); // <-- CHANGED from logTextBrowser to logTextEdit
    QScrollBar *sb = ui->logTextEdit->verticalScrollBar(); // <-- CHANGED here too
    if (sb) {
        sb->setValue(sb->maximum());
    }
    qDebug() << "LOG:" << message; // Also output to debug console
}

void DownloadProgressDialog::setStatus(const QString &status)
{
    ui->statusLabel->setText(status); // <-- CHANGED from currentStatusLabel to statusLabel
    qDebug() << "STATUS:" << status;
}

void DownloadProgressDialog::updateOverallProgress()
{
    ui->progressBarOverall->setValue(currentRepoIndex);
    ui->progressBarOverall->setFormat(tr("%v/%m repositories processed"));
}