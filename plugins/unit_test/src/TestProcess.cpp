#include "TestProcess.h"
#include <QRegularExpression>

TestProcess::TestProcess(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, &TestProcess::onReadyRead);
    connect(m_process, &QProcess::readyReadStandardError,  this, &TestProcess::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TestProcess::onFinished);
}

void TestProcess::run(const QString &binaryPath, const QStringList &extraArgs)
{
    if (m_process->state() != QProcess::NotRunning)
        return;

    m_xmlBuffer.clear();
    m_currentTestCase.clear();

    // Ask QTest for XML output so we can parse structured results.
    QStringList args = extraArgs;
    args << "-xml";

    m_process->setProgram(binaryPath);
    m_process->setArguments(args);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->start();

    if (m_process->waitForStarted(3000))
        emit started();
}

void TestProcess::kill()
{
    m_process->kill();
}

bool TestProcess::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void TestProcess::onReadyRead()
{
    const QString chunk = QString::fromUtf8(m_process->readAll());
    m_xmlBuffer += chunk;

    // Emit raw lines for the log panel
    for (const QString &line : chunk.split('\n'))
        if (!line.trimmed().isEmpty())
            emit logLine(line.trimmed());

    // Parse completed XML elements line-by-line
    int pos = 0;
    while (true) {
        int nl = m_xmlBuffer.indexOf('\n', pos);
        if (nl == -1) break;
        parseLine(m_xmlBuffer.mid(pos, nl - pos).trimmed());
        pos = nl + 1;
    }
    m_xmlBuffer = m_xmlBuffer.mid(pos);
}

void TestProcess::onFinished(int exitCode, QProcess::ExitStatus)
{
    emit finished(exitCode);
}

void TestProcess::parseLine(const QString &line)
{
    // Extract TestCase name
    {
        static QRegularExpression reTC("<TestCase name=\"([^\"]+)\"");
        auto m = reTC.match(line);
        if (m.hasMatch()) {
            m_currentTestCase = m.captured(1);
            return;
        }
    }

    // Simpler single-line match approach used for robustness:
    // Match lines like: PASS   : MyTests::testAddition()
    //                   FAIL!  : MyTests::testAddition() 'x == 1' returned FALSE
    {
        static QRegularExpression reSimple(
            "^(PASS|FAIL!|SKIP|XFAIL|XPASS)\\s+:\\s+([\\w:]+)\\(\\)(.*)");
        auto m = reSimple.match(line);
        if (m.hasMatch()) {
            TestResult r;
            r.testCaseName = m_currentTestCase;
            QString rawResult = m.captured(1);
            if (rawResult == "FAIL!") rawResult = "FAIL";
            r.result       = rawResult;
            r.functionName = m.captured(2);
            r.message      = m.captured(3).trimmed();
            emit resultReady(r);
        }
    }

    // Also catch "Totals:" lines for timing info (emitted as log)
}
