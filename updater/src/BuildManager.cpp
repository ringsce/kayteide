#include "BuildManager.h"

#include <QDir>
#include <QFileInfo>

BuildManager::BuildManager(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onStdOut);
    connect(m_process, &QProcess::readyReadStandardError,  this, &BuildManager::onStdErr);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BuildManager::onProcessFinished);

    // Sensible defaults using the system temp dir
    if (m_sourceDir.isEmpty())
        m_sourceDir = QDir::tempPath() + "/kayteide-src";
    if (m_buildDir.isEmpty())
        m_buildDir  = QDir::tempPath() + "/kayteide-build";
}

bool BuildManager::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

bool BuildManager::repoExists() const
{
    return QFileInfo::exists(m_sourceDir + "/.git");
}

void BuildManager::startBuild()
{
    if (isRunning()) {
        emit logLine("Build already in progress — aborting previous build first.", true);
        abort();
        return;
    }

    m_pipelineStage = 0;
    advancePipeline();
}

void BuildManager::abort()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(2000);
        emit logLine("⚠ Build aborted by user.", true);
        m_step = BuildStep::Idle;
    }
}

void BuildManager::advancePipeline()
{
    QDir().mkpath(m_sourceDir);
    QDir().mkpath(m_buildDir);

    switch (m_pipelineStage) {
    case 0:
        // Clone or pull
        if (repoExists()) {
            runStep(BuildStep::Pulling,
                    "git", {"pull", "--rebase", "--autostash"},
                    m_sourceDir);
        } else {
            runStep(BuildStep::Cloning,
                    "git", {"clone", "--depth=1", m_repoUrl, m_sourceDir},
                    QDir::tempPath());
        }
        break;

    case 1:
        // CMake configure
        runStep(BuildStep::Configuring,
                "cmake", {
                    "-S", m_sourceDir,
                    "-B", m_buildDir,
                    "-DCMAKE_BUILD_TYPE=Release"
                });
        break;

    case 2:
        // CMake build
        runStep(BuildStep::Building,
                "cmake", {
                    "--build", m_buildDir,
                    "--parallel", QString::number(m_jobs),
                    "--config", "Release"
                });
        break;

    case 3:
        // Optional install
        if (m_runInstall && !m_installDir.isEmpty()) {
            runStep(BuildStep::Installing,
                    "cmake", {
                        "--install", m_buildDir,
                        "--prefix", m_installDir
                    });
        } else {
            // Skip to done
            m_step = BuildStep::Done;
            emit progressChanged(100);
            emit stepChanged(BuildStep::Done, "Build complete!");
            emit buildSucceeded();
        }
        break;

    case 4:
        m_step = BuildStep::Done;
        emit progressChanged(100);
        emit stepChanged(BuildStep::Done, "Install complete!");
        emit buildSucceeded();
        break;

    default:
        break;
    }
}

void BuildManager::runStep(BuildStep next, const QString &program,
                            const QStringList &args, const QString &workDir)
{
    m_step = next;

    static const QMap<BuildStep, QPair<QString,int>> meta{
        {BuildStep::Cloning,      {"Cloning repository…",   10}},
        {BuildStep::Pulling,      {"Pulling latest code…",  10}},
        {BuildStep::Configuring,  {"Configuring with CMake…",40}},
        {BuildStep::Building,     {"Building KayteIDE…",    80}},
        {BuildStep::Installing,   {"Installing…",            95}},
    };

    auto [desc, pct] = meta.value(next, {"Working…", 50});
    emit stepChanged(next, desc);
    emit progressChanged(pct);
    emit logLine(QString("▶ %1 %2").arg(program, args.join(' ')));

    if (!workDir.isEmpty())
        m_process->setWorkingDirectory(workDir);

    m_process->start(program, args);
}

void BuildManager::onStdOut()
{
    const QString text = QString::fromUtf8(m_process->readAllStandardOutput());
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts))
        emit logLine(line.trimmed());
}

void BuildManager::onStdErr()
{
    const QString text = QString::fromUtf8(m_process->readAllStandardError());
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts))
        emit logLine(line.trimmed(), /*isError=*/true);
}

void BuildManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit || exitCode != 0) {
        m_step = BuildStep::Failed;
        const QString reason = QString("Step failed (exit code %1).").arg(exitCode);
        emit stepChanged(BuildStep::Failed, reason);
        emit buildFailed(reason);
        return;
    }

    ++m_pipelineStage;
    advancePipeline();
}
