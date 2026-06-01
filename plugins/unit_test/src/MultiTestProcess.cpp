#include "MultiTestProcess.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QDir>

MultiTestProcess::MultiTestProcess(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_tmpDir(new QTemporaryDir())
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &MultiTestProcess::onRunReadyRead);
}

MultiTestProcess::~MultiTestProcess()
{
    delete m_tmpDir;
}

void MultiTestProcess::run(const QString &filePath)
{
    if (m_process->state() != QProcess::NotRunning) return;

    m_filePath   = filePath;
    m_runner     = LanguageRunner::fromFile(filePath);
    m_testIndex  = 0;
    m_anyFail    = false;
    m_outputBin  = QString();

    emit started(m_runner.language);
    emitLog(QString("▶ Running %1 [%2]").arg(QFileInfo(filePath).fileName(), m_runner.language),
            "#89b4fa");

    if (m_runner.mode == LanguageRunner::Mode::Compiled ||
        m_runner.mode == LanguageRunner::Mode::HeaderOnly)
        startCompile();
    else
        startRun();
}

void MultiTestProcess::kill()
{
    m_process->kill();
}

bool MultiTestProcess::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

// ── Compile step ─────────────────────────────────────────────────────────────

void MultiTestProcess::startCompile()
{
    const QString src  = m_filePath;
    const QString base = QFileInfo(src).completeBaseName();

    if (m_runner.mode == LanguageRunner::Mode::HeaderOnly) {
        // Syntax-only check — no output binary
        QStringList args = m_runner.compilerFlags;
        args << src;
        m_process->disconnect(SIGNAL(finished(int,QProcess::ExitStatus)));
        connect(m_process,
                QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
                this, &MultiTestProcess::onCompileFinished);
        m_process->start(m_runner.compiler, args);
        return;
    }

    // Full compile
    m_outputBin = m_tmpDir->filePath(base);
#ifdef Q_OS_WIN
    m_outputBin += ".exe";
#endif

    QStringList args = m_runner.compilerFlags;
    // For gcc/g++, append "-o <bin> <src>"; for fpc append "<src>" only
    if (m_runner.compiler == "fpc") {
        args << src << QString("-o%1").arg(m_outputBin);
    } else {
        args << m_outputBin << src;
    }

    m_process->disconnect(SIGNAL(finished(int,QProcess::ExitStatus)));
    connect(m_process,
            QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MultiTestProcess::onCompileFinished);
    m_process->start(m_runner.compiler, args);
}

void MultiTestProcess::onCompileFinished(int exitCode, QProcess::ExitStatus)
{
    const QString out = QString::fromUtf8(m_process->readAll());
    for (const QString &line : out.split('\n'))
        if (!line.trimmed().isEmpty())
            parseOutputLine(line.trimmed(), "compile");

    if (exitCode != 0) {
        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = "compile";
        r.result       = "FAIL";
        r.message      = QString("Compilation failed (exit %1)").arg(exitCode);
        emit resultReady(r);
        emit finished(exitCode);
        return;
    }

    // Compile succeeded
    if (m_runner.mode == LanguageRunner::Mode::HeaderOnly) {
        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = "syntax-check";
        r.result       = "PASS";
        r.message      = "No syntax errors";
        emit resultReady(r);
        emit finished(0);
        return;
    }

    emitLog("✓ Compiled successfully", "#a6e3a1");
    {
        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = "compile";
        r.result       = "PASS";
        r.message      = "Compiled OK";
        emit resultReady(r);
    }
    startRun();
}

// ── Run step ──────────────────────────────────────────────────────────────────

void MultiTestProcess::startRun()
{
    m_process->disconnect(SIGNAL(finished(int,QProcess::ExitStatus)));
    connect(m_process,
            QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MultiTestProcess::onRunFinished);

    if (m_runner.mode == LanguageRunner::Mode::Compiled) {
        m_process->start(m_outputBin, {});
    } else {
        // Interpreted
        QStringList args = m_runner.interpreterFlags;
        args << m_filePath;
        m_process->start(m_runner.interpreter, args);
    }
}

void MultiTestProcess::onRunReadyRead()
{
    const QString chunk = QString::fromUtf8(m_process->readAll());
    for (const QString &line : chunk.split('\n'))
        if (!line.trimmed().isEmpty())
            parseOutputLine(line.trimmed(), "run");
}

void MultiTestProcess::onRunFinished(int exitCode, QProcess::ExitStatus)
{
    // Flush remaining output
    const QString remaining = QString::fromUtf8(m_process->readAll());
    for (const QString &line : remaining.split('\n'))
        if (!line.trimmed().isEmpty())
            parseOutputLine(line.trimmed(), "run");

    // Emit an overall "exit" result
    TestResult r;
    r.testCaseName = QFileInfo(m_filePath).fileName();
    r.functionName = "exit";
    r.result       = (exitCode == 0) ? "PASS" : "FAIL";
    r.message      = (exitCode == 0)
                     ? "Process exited cleanly"
                     : QString("Process exited with code %1").arg(exitCode);
    emit resultReady(r);
    emit finished(exitCode);
}

// ── Output parser ─────────────────────────────────────────────────────────────

void MultiTestProcess::parseOutputLine(const QString &raw, const QString &context)
{
    emit logLine(raw.toHtmlEscaped());

    // Detect explicit PASS/FAIL patterns that test scripts may print
    static QRegularExpression rePassFail(
        "\\b(PASS|FAIL|OK|ERROR|WARNING|SKIP)\\b[:\\s]*(.*)",
        QRegularExpression::CaseInsensitiveOption);
    auto m = rePassFail.match(raw);
    if (m.hasMatch()) {
        QString keyword = m.captured(1).toUpper();
        QString msg     = m.captured(2).trimmed();

        // Normalise
        if (keyword == "OK")      keyword = "PASS";
        if (keyword == "ERROR")   keyword = "FAIL";
        if (keyword == "WARNING") keyword = "SKIP";  // treat warnings as skipped

        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = QString("test_%1").arg(++m_testIndex);
        r.result       = keyword;
        r.message      = msg.isEmpty() ? raw : msg;
        emit resultReady(r);
        return;
    }

    // Detect compiler-style warnings/errors: file:line: error: …
    static QRegularExpression reCompilerMsg(
        "([^:]+):(\\d+):\\s*(warning|error|note):\\s*(.*)",
        QRegularExpression::CaseInsensitiveOption);
    auto m2 = reCompilerMsg.match(raw);
    if (m2.hasMatch() && context == "compile") {
        QString severity = m2.captured(3).toLower();
        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = QString("line_%1").arg(m2.captured(2));
        r.result       = (severity == "error") ? "FAIL" : "SKIP";
        r.message      = m2.captured(4).trimmed();
        r.file         = m2.captured(1);
        r.line         = m2.captured(2).toInt();
        emit resultReady(r);
        return;
    }

    // PHP errors: PHP Fatal error / PHP Parse error
    static QRegularExpression rePhp(
        "PHP (Fatal error|Parse error|Warning|Notice):\\s*(.*)",
        QRegularExpression::CaseInsensitiveOption);
    auto m3 = rePhp.match(raw);
    if (m3.hasMatch()) {
        QString sev = m3.captured(1).toLower();
        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = QString("php_%1").arg(++m_testIndex);
        r.result       = (sev.contains("error")) ? "FAIL" : "SKIP";
        r.message      = m3.captured(2).trimmed();
        emit resultReady(r);
        return;
    }

    // Perl: "... at file.pl line N"
    static QRegularExpression rePerl("(.+) at .+ line (\\d+)");
    auto m4 = rePerl.match(raw);
    if (m4.hasMatch()) {
        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = QString("line_%1").arg(m4.captured(2));
        r.result       = "FAIL";
        r.message      = m4.captured(1).trimmed();
        r.line         = m4.captured(2).toInt();
        emit resultReady(r);
        return;
    }

    // Kayte: treat any PRINT output line as an informational PASS
    if (m_runner.language == "Kayte") {
        TestResult r;
        r.testCaseName = QFileInfo(m_filePath).fileName();
        r.functionName = QString("output_%1").arg(++m_testIndex);
        r.result       = "PASS";
        r.message      = raw;
        emit resultReady(r);
    }
}

void MultiTestProcess::emitLog(const QString &text, const QString &color)
{
    emit logLine(QString("<span style='color:%1'>%2</span>")
                 .arg(color, text.toHtmlEscaped()));
}
