#ifndef DOWNLOADPROGRESSDIALOG_H
#define DOWNLOADPROGRESSDIALOG_H

#include <QDialog>
#include <QProcess>
#include <QStringList>
#include <QVariantMap> // Or just use a struct if preferred for GitHubRepo

// Define a struct to hold GitHub repository information
struct GitHubRepo {
    QString url;
    QString localDirName;
    QString branch; // Optional branch
};

// Include the namespace for Ui::DownloadProgressDialog
// This is what was missing and causing the "no type named 'DownloadProgressDialog' in namespace 'Ui'" error
QT_BEGIN_NAMESPACE
namespace Ui { class DownloadProgressDialog; }
QT_END_NAMESPACE

class DownloadProgressDialog : public QDialog
{
    Q_OBJECT

public:
    // Define stages for the download/prerequisite process
    enum ProcessStage {
        InitialStage,          // Before any checks begin
        XcodeToolsCheckStage,  // Checking if Xcode Command Line Tools are installed
        BrewCheckStage,        // Checking if Homebrew is installed
        BrewInstallStage,      // Installing Homebrew (if not found)
        ToolCheckStage,        // Checking for specific tools (git, cmake, make)
        ToolInstallStage,      // Installing missing specific tools
        DownloadStage          // Actual repository download phase
    };
    Q_ENUM(ProcessStage) // Makes ProcessStage visible to QMetaobject system (useful for debugging)

    explicit DownloadProgressDialog(QWidget *parent = nullptr);
    ~DownloadProgressDialog();

    // Call this from MainWindow to initiate the overall process
    // This will implicitly trigger prerequisite checks first.
    void startProcess(const QStringList &repos, const QString &downloadDir);

signals:
    // Emitted when all system prerequisites are met AND downloads are completed
    void processCompleted();
    // Emitted if the process is cancelled or a critical error occurs
    void processAborted();

private slots:
    // ... existing slots ...
    void handleSingleRepoDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleCancelButtonClicked();
    void handleProcessStarted();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus); // Unified handler for QProcess::finished
    void handleProcessReadyReadStandardOutput();
    void handleProcessReadyReadStandardError();
    void handleProcessErrorOccurred(QProcess::ProcessError error);

    // Stage-specific handlers for QProcess::finished
    void handleXcodeToolsCheckFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleBrewCheckFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleBrewInstallFinished(int exitCode, QProcess::ExitStatus exitStatus); // New dedicated slot for Brew install
    void handleToolCheckFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleToolInstallFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::DownloadProgressDialog *ui;
    QProcess *process; // Unified QProcess instance

    // Overall process state
    ProcessStage currentStage;
    QList<GitHubRepo> reposToDownload;
    QString downloadDestinationDir;
    int currentRepoIndex;

    // System tool checking/installation state
    QStringList systemToolsRequired; // e.g., "git", "cmake", "make"
    int currentSystemToolCheckIndex;
    QStringList foundTools;
    QStringList missingTools; // Tools that need to be installed
    int currentMissingToolInstallIndex; // For iterating through missingTools for installation
    void installNextMissingTool(); // <--- ADD THIS
    void checkNextTool(); // <--- Make sure this line exists!


    bool homebrewIsInstalled; // Track Homebrew status

    // Helper functions for various stages
    void setupProcessEnvironment(); // Sets up PATH etc.
    void startXcodeToolsCheck();
    void startBrewCheck();
    void startBrewInstall();
    void startToolChecks(); // Checks git, cmake, make etc.
    void startToolInstall(); // Installs one missing tool
    void finishAllChecksAndStartDownloads(); // Called when all tools are ready

    // Helper for main download loop
    void processNextRepo();

    // UI update helpers
    void log(const QString &message);
    void setStatus(const QString &status);
    void updateOverallProgress();
};

#endif // DOWNLOADPROGRESSDIALOG_H