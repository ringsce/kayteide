#pragma once
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

// Represents one test case result parsed from QTest XML output.
struct TestResult {
    QString testCaseName;   // e.g. "MyTests"
    QString functionName;   // e.g. "testAddition"
    QString result;         // "PASS" | "FAIL" | "SKIP" | "XFAIL" | "XPASS"
    QString message;        // failure description (empty on pass)
    QString file;
    int     line = -1;
    qint64  durationMs = 0;
};

class TestProcess : public QObject
{
    Q_OBJECT
public:
    explicit TestProcess(QObject *parent = nullptr);

    // Start the test binary located at `binaryPath`.
    // Extra QTest args can be forwarded via `extraArgs`.
    void run(const QString &binaryPath, const QStringList &extraArgs = {});
    void kill();

    bool isRunning() const;

signals:
    void started();
    void resultReady(const TestResult &result);
    void logLine(const QString &line);
    void finished(int exitCode);

private slots:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);

private:
    void parseLine(const QString &line);

    QProcess   *m_process;
    QString     m_xmlBuffer;
    QString     m_currentTestCase;
};
