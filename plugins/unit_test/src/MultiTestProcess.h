#pragma once
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include "LanguageRunner.h"
#include "TestProcess.h"   // reuse TestResult

// Runs a single source file through the appropriate toolchain,
// streaming TestResult signals just like TestProcess does.
class MultiTestProcess : public QObject
{
    Q_OBJECT
public:
    explicit MultiTestProcess(QObject *parent = nullptr);
    ~MultiTestProcess() override;

    void run(const QString &filePath);
    void kill();
    bool isRunning() const;

signals:
    void started(const QString &language);
    void resultReady(const TestResult &result);
    void logLine(const QString &html);   // pre-coloured HTML line
    void finished(int exitCode);

private slots:
    void onCompileFinished(int exitCode, QProcess::ExitStatus);
    void onRunReadyRead();
    void onRunFinished(int exitCode, QProcess::ExitStatus);

private:
    void startCompile();
    void startRun();
    void parseOutputLine(const QString &raw, const QString &context);
    void emitLog(const QString &text, const QString &color = "#a6adc8");

    QString          m_filePath;
    LanguageRunner   m_runner;
    QProcess        *m_process    = nullptr;
    QTemporaryDir   *m_tmpDir     = nullptr;
    QString          m_outputBin;      // path of compiled binary
    int              m_testIndex  = 0;
    bool             m_anyFail    = false;
};
