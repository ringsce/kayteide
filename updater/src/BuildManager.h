#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

enum class BuildStep {
    Idle,
    Cloning,
    Pulling,
    Configuring,   // cmake configure
    Building,      // cmake --build
    Installing,    // cmake --install (optional)
    Done,
    Failed
};

class BuildManager : public QObject
{
    Q_OBJECT

public:
    explicit BuildManager(QObject *parent = nullptr);
    ~BuildManager() override = default;

    // Paths
    void setRepoUrl(const QString &url)      { m_repoUrl = url; }
    void setSourceDir(const QString &dir)    { m_sourceDir = dir; }
    void setBuildDir(const QString &dir)     { m_buildDir = dir; }
    void setInstallDir(const QString &dir)   { m_installDir = dir; }
    void setMakeJobs(int jobs)               { m_jobs = jobs; }
    void setRunInstall(bool run)             { m_runInstall = run; }

    BuildStep currentStep() const           { return m_step; }
    bool isRunning() const;

public slots:
    void startBuild();   // full clone-or-pull + build pipeline
    void abort();

signals:
    void stepChanged(BuildStep step, const QString &description);
    void logLine(const QString &line, bool isError = false);
    void progressChanged(int percent); // 0-100
    void buildSucceeded();
    void buildFailed(const QString &reason);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onStdOut();
    void onStdErr();

private:
    void runStep(BuildStep next, const QString &program,
                 const QStringList &args, const QString &workDir = {});
    void advancePipeline();
    bool repoExists() const;

    QProcess  *m_process   = nullptr;
    BuildStep  m_step      = BuildStep::Idle;
    int        m_jobs      = 4;
    bool       m_runInstall = false;

    QString m_repoUrl    = "https://github.com/ringsce/kayteide.git";
    QString m_sourceDir;
    QString m_buildDir;
    QString m_installDir;

    // Pipeline queue
    int m_pipelineStage = 0;
};
