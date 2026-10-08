#include "CliAgentClient.hpp"
#include "LlmLog.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QUuid>

namespace Kayte::Llm {

namespace {

// GUI apps on macOS get a minimal PATH; the CLIs usually live in these.
QStringList extraBinDirs()
{
    const QString home = QDir::homePath();
    return { home + QStringLiteral("/.local/bin"), home + QStringLiteral("/.claude/local"),
             home + QStringLiteral("/.npm-global/bin"), home + QStringLiteral("/.bun/bin"),
             QStringLiteral("/opt/homebrew/bin"), QStringLiteral("/usr/local/bin"),
             QStringLiteral("/usr/bin") };
}

QProcessEnvironment cliEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // The Codex CLI is a node script; make sure node and friends resolve.
    QStringList path = env.value(QStringLiteral("PATH")).split(QDir::listSeparator(),
                                                               Qt::SkipEmptyParts);
    for (const QString &d : extraBinDirs())
        if (!path.contains(d)) path << d;
    env.insert(QStringLiteral("PATH"), path.join(QDir::listSeparator()));
    // If KayteIDE itself was started from a Claude Code session, don't let the
    // child think it is nested inside one.
    env.remove(QStringLiteral("CLAUDECODE"));
    return env;
}

QString shellQuote(const QString &s)
{
    QString q = s;
    q.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + q + QLatin1Char('\'');
}

} // namespace

CliAgentClient::CliAgentClient(QObject *parent)
    : QObject(parent)
{
}

CliAgentClient::~CliAgentClient()
{
    for (auto it = m_jobs.begin(); it != m_jobs.end(); ++it) {
        it->process->disconnect(this);
        it->process->kill();
        it->process->waitForFinished(1000);
        if (!it->lastMessageFile.isEmpty()) QFile::remove(it->lastMessageFile);
    }
}

// ─── Discovery / sign-in ──────────────────────────────────────────────────────

QString CliAgentClient::name(Provider p)
{
    return p == Provider::Claude ? QStringLiteral("Claude") : QStringLiteral("Codex");
}

QString CliAgentClient::executable(Provider p)
{
    const QString exe = p == Provider::Claude ? QStringLiteral("claude") : QStringLiteral("codex");
    QString found = QStandardPaths::findExecutable(exe);
    if (found.isEmpty()) found = QStandardPaths::findExecutable(exe, extraBinDirs());
    return found;
}

QString CliAgentClient::loginCommand(Provider p)
{
    const QString exe = executable(p);
    if (exe.isEmpty()) return {};
    return shellQuote(exe) + (p == Provider::Claude ? QStringLiteral(" auth login")
                                                    : QStringLiteral(" login"));
}

QString CliAgentClient::installCommand(Provider p)
{
    return p == Provider::Claude
        ? QStringLiteral("curl -fsSL https://claude.ai/install.sh | bash")
        : QStringLiteral("npm install -g @openai/codex");
}

void CliAgentClient::checkLogin(Provider p)
{
    const QString exe = executable(p);
    if (exe.isEmpty()) {
        LlmLog::write(name(p), QStringLiteral("CLI not installed"));
        emit loginStatus(p, false, false, tr("%1 CLI is not installed").arg(name(p)));
        return;
    }

    auto *proc = new QProcess(this);
    proc->setProcessEnvironment(cliEnvironment());
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::finished, this, [this, proc, p](int code, QProcess::ExitStatus) {
        const QByteArray out = proc->readAll();
        proc->deleteLater();

        bool loggedIn = false;
        QString detail;
        if (p == Provider::Claude) {
            const QJsonObject o = QJsonDocument::fromJson(out).object();
            loggedIn = o.value(QStringLiteral("loggedIn")).toBool();
            if (loggedIn) {
                detail = QStringLiteral("%1 (%2%3)")
                    .arg(o.value(QStringLiteral("email")).toString(),
                         o.value(QStringLiteral("authMethod")).toString(),
                         o.contains(QStringLiteral("subscriptionType"))
                             ? QStringLiteral(", ") + o.value(QStringLiteral("subscriptionType")).toString()
                             : QString());
            }
        } else {
            loggedIn = code == 0;
            detail = QString::fromUtf8(out).trimmed().section(QLatin1Char('\n'), -1);
        }
        LlmLog::write(name(p), loggedIn ? QStringLiteral("signed in: %1").arg(detail)
                                        : QStringLiteral("not signed in"));
        emit loginStatus(p, true, loggedIn, detail);
    });
    proc->start(exe, p == Provider::Claude
                         ? QStringList { QStringLiteral("auth"), QStringLiteral("status"),
                                         QStringLiteral("--json") }
                         : QStringList { QStringLiteral("login"), QStringLiteral("status") });
}

void CliAgentClient::openTerminal(const QString &command)
{
    LlmLog::write(QStringLiteral("terminal"), command);
#if defined(Q_OS_MACOS)
    QString escaped = command;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"))
           .replace(QLatin1Char('"'), QStringLiteral("\\\""));
    QProcess::startDetached(QStringLiteral("osascript"),
        { QStringLiteral("-e"), QStringLiteral("tell application \"Terminal\""),
          QStringLiteral("-e"), QStringLiteral("activate"),
          QStringLiteral("-e"), QStringLiteral("do script \"%1\"").arg(escaped),
          QStringLiteral("-e"), QStringLiteral("end tell") });
#elif defined(Q_OS_WIN)
    QProcess::startDetached(QStringLiteral("cmd.exe"),
        { QStringLiteral("/c"), QStringLiteral("start"), QStringLiteral("cmd"),
          QStringLiteral("/k"), command });
#else
    const QString shell = command + QStringLiteral("; exec bash");
    for (const auto &term : { QStringList { QStringLiteral("x-terminal-emulator"), QStringLiteral("-e") },
                              QStringList { QStringLiteral("gnome-terminal"), QStringLiteral("--") },
                              QStringList { QStringLiteral("konsole"), QStringLiteral("-e") },
                              QStringList { QStringLiteral("xterm"), QStringLiteral("-e") } }) {
        if (QStandardPaths::findExecutable(term.first()).isEmpty()) continue;
        QStringList args = term.mid(1);
        args << QStringLiteral("bash") << QStringLiteral("-lc") << shell;
        if (QProcess::startDetached(term.first(), args)) return;
    }
#endif
}

// ─── Running a request ────────────────────────────────────────────────────────

void CliAgentClient::run(const QString &tag, Provider p, const QString &model,
                         const QString &prompt, const QString &workingDir)
{
    abort(tag);

    const QString exe = executable(p);
    if (exe.isEmpty()) {
        emit failed(tag, tr("%1 CLI is not installed.").arg(name(p)));
        return;
    }

    Job job;
    job.provider = p;
    job.model    = model;
    job.process  = new QProcess(this);
    job.process->setProcessEnvironment(cliEnvironment());
    if (!workingDir.isEmpty() && QDir(workingDir).exists())
        job.process->setWorkingDirectory(workingDir);

    QStringList args;
    if (p == Provider::Claude) {
        // Read-only: Claude may read/search the project but never edit or run.
        args << QStringLiteral("-p")
             << QStringLiteral("--output-format") << QStringLiteral("stream-json")
             << QStringLiteral("--verbose") << QStringLiteral("--include-partial-messages")
             << QStringLiteral("--no-session-persistence");
        if (!model.isEmpty()) args << QStringLiteral("--model") << model;
        args << QStringLiteral("--tools") << QStringLiteral("Read")
             << QStringLiteral("Grep") << QStringLiteral("Glob");
    } else {
        job.lastMessageFile = QDir::temp().filePath(
            QStringLiteral("kayte-codex-%1.txt").arg(QUuid::createUuid().toString(QUuid::Id128)));
        args << QStringLiteral("exec") << QStringLiteral("--json")
             << QStringLiteral("--skip-git-repo-check") << QStringLiteral("--ephemeral")
             << QStringLiteral("--sandbox") << QStringLiteral("read-only")
             << QStringLiteral("--output-last-message") << job.lastMessageFile;
        if (!workingDir.isEmpty()) args << QStringLiteral("--cd") << workingDir;
        if (!model.isEmpty()) args << QStringLiteral("--model") << model;
    }

    QProcess *proc = job.process;
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, tag] { onReadyRead(tag); });
    connect(proc, &QProcess::readyReadStandardError, this, [this, tag, proc] {
        auto it = m_jobs.find(tag);
        if (it == m_jobs.end() || it->process != proc) return;
        it->stderrTail = (it->stderrTail + proc->readAllStandardError()).right(4000);
    });
    connect(proc, &QProcess::finished, this,
            [this, tag, proc](int code, QProcess::ExitStatus st) {
        auto it = m_jobs.find(tag);
        if (it == m_jobs.end() || it->process != proc) { proc->deleteLater(); return; }
        onFinished(tag, code, st);
    });

    job.timer.start();
    m_jobs.insert(tag, job);
    LlmLog::write(name(p), QStringLiteral("request [%1] model=%2 cwd=%3 prompt=%4 chars")
                               .arg(tag, model.isEmpty() ? QStringLiteral("default") : model,
                                    workingDir).arg(prompt.size()));

    // Asynchronous: feed the prompt once the process is up; never block the GUI.
    connect(proc, &QProcess::started, this, [proc, prompt] {
        proc->write(prompt.toUtf8());
        proc->closeWriteChannel();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, tag, proc, exe, p](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        auto it = m_jobs.find(tag);
        if (it == m_jobs.end() || it->process != proc) return;
        m_jobs.erase(it);
        proc->deleteLater();
        LlmLog::write(name(p), QStringLiteral("failed to start %1").arg(exe));
        emit failed(tag, tr("Could not start %1.").arg(exe));
    });
    proc->start(exe, args);
}

void CliAgentClient::abort(const QString &tag)
{
    auto it = m_jobs.find(tag);
    if (it == m_jobs.end()) return;
    Job job = *it;
    m_jobs.erase(it);
    job.process->disconnect(this);
    job.process->kill();
    job.process->waitForFinished(1000);
    job.process->deleteLater();
    if (!job.lastMessageFile.isEmpty()) QFile::remove(job.lastMessageFile);
    LlmLog::write(name(job.provider), QStringLiteral("aborted [%1]").arg(tag));
}

void CliAgentClient::onReadyRead(const QString &tag)
{
    auto it = m_jobs.find(tag);
    if (it == m_jobs.end()) return;
    it->buffer += it->process->readAllStandardOutput();
    int nl;
    while ((nl = it->buffer.indexOf('\n')) >= 0) {
        const QByteArray line = it->buffer.left(nl).trimmed();
        it->buffer.remove(0, nl + 1);
        if (line.isEmpty()) continue;
        const QJsonObject event = QJsonDocument::fromJson(line).object();
        if (event.isEmpty()) continue;
        if (it->provider == Provider::Claude) handleClaudeEvent(tag, *it, event);
        else                                  handleCodexEvent(tag, *it, event);
        it = m_jobs.find(tag);           // a handler never removes, but stay safe
        if (it == m_jobs.end()) return;
    }
}

// claude -p --output-format stream-json --include-partial-messages
void CliAgentClient::handleClaudeEvent(const QString &tag, Job &job, const QJsonObject &e)
{
    const QString type = e.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("stream_event")) {
        const QJsonObject ev = e.value(QStringLiteral("event")).toObject();
        const QString evType = ev.value(QStringLiteral("type")).toString();
        if (evType == QLatin1String("content_block_delta")) {
            const QJsonObject d = ev.value(QStringLiteral("delta")).toObject();
            if (d.value(QStringLiteral("type")).toString() == QLatin1String("text_delta")) {
                const QString t = d.value(QStringLiteral("text")).toString();
                job.text += t;
                emit token(tag, t);
            }
        } else if (evType == QLatin1String("content_block_start")) {
            const QJsonObject b = ev.value(QStringLiteral("content_block")).toObject();
            if (b.value(QStringLiteral("type")).toString() == QLatin1String("tool_use")) {
                emit activity(tag, tr("Claude is using %1 on the project…")
                                       .arg(b.value(QStringLiteral("name")).toString()));
                // Keep the separate text segments around tool calls readable.
                if (!job.text.isEmpty() && !job.text.endsWith(QLatin1Char('\n'))) {
                    job.text += QStringLiteral("\n\n");
                    emit token(tag, QStringLiteral("\n\n"));
                }
            }
        }
    } else if (type == QLatin1String("result")) {
        job.costUsd = e.value(QStringLiteral("total_cost_usd")).toDouble(-1);
        if (e.value(QStringLiteral("is_error")).toBool())
            job.error = e.value(QStringLiteral("result")).toString();
        else if (job.text.trimmed().isEmpty())
            job.text = e.value(QStringLiteral("result")).toString();
    }
}

// codex exec --json. Accepts both the item-based and the older msg-based event
// formats; the --output-last-message file is the final word either way.
void CliAgentClient::handleCodexEvent(const QString &tag, Job &job, const QJsonObject &e)
{
    const QString type = e.value(QStringLiteral("type")).toString();
    const QJsonObject item = e.value(QStringLiteral("item")).toObject();
    const QString itemType = item.value(QStringLiteral("type")).toString();
    const QJsonObject msg = e.value(QStringLiteral("msg")).toObject();
    const QString msgType = msg.value(QStringLiteral("type")).toString();

    // item.updated / item.completed carry the whole message so far: emit only
    // what is new, and start a new paragraph when a different message begins.
    auto setItemText = [&](const QString &id, const QString &full) {
        if (id != job.itemId || id.isEmpty()) {
            if (!job.text.isEmpty() && !job.text.endsWith(QLatin1Char('\n'))) {
                job.text += QStringLiteral("\n\n");
                emit token(tag, QStringLiteral("\n\n"));
            }
            job.itemId    = id;
            job.itemStart = job.text.size();
        }
        const QString shown = job.text.mid(job.itemStart);
        if (full.startsWith(shown) && full.size() > shown.size()) {
            const QString delta = full.mid(shown.size());
            job.text += delta;
            emit token(tag, delta);
        }
    };

    if ((type == QLatin1String("item.completed") || type == QLatin1String("item.updated")) &&
        itemType == QLatin1String("agent_message")) {
        setItemText(item.value(QStringLiteral("id")).toString(),
                    item.value(QStringLiteral("text")).toString());
    } else if (type == QLatin1String("item.started") &&
               (itemType == QLatin1String("command_execution") ||
                itemType == QLatin1String("file_change"))) {
        emit activity(tag, tr("Codex is inspecting the project…"));
    } else if (msgType == QLatin1String("agent_message_delta")) {
        const QString d = msg.value(QStringLiteral("delta")).toString();
        job.text += d;
        emit token(tag, d);
    } else if (msgType == QLatin1String("agent_message")) {
        const QString full = msg.value(QStringLiteral("message")).toString();
        if (!job.text.contains(full)) setItemText(QString(), full);
    } else if (type == QLatin1String("error") || msgType == QLatin1String("error")) {
        job.error = e.value(QStringLiteral("message")).toString(
                        msg.value(QStringLiteral("message")).toString());
    } else if (type == QLatin1String("turn.failed")) {
        job.error = e.value(QStringLiteral("error")).toObject()
                        .value(QStringLiteral("message")).toString(job.error);
    }
}

void CliAgentClient::onFinished(const QString &tag, int exitCode, QProcess::ExitStatus status)
{
    onReadyRead(tag);   // drain
    Job job = m_jobs.take(tag);
    job.process->deleteLater();
    if (!job.buffer.trimmed().isEmpty()) {
        const QJsonObject e = QJsonDocument::fromJson(job.buffer).object();
        if (job.provider == Provider::Claude) handleClaudeEvent(tag, job, e);
        else                                  handleCodexEvent(tag, job, e);
    }

    if (!job.lastMessageFile.isEmpty()) {
        QFile f(job.lastMessageFile);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString last = QString::fromUtf8(f.readAll()).trimmed();
            if (!last.isEmpty() && !job.text.contains(last)) {
                const QString add = job.text.isEmpty() ? last : QStringLiteral("\n\n") + last;
                job.text += add;
                emit token(tag, add);
            }
        }
        QFile::remove(job.lastMessageFile);
    }

    const double secs = job.timer.elapsed() / 1000.0;
    const bool ok = status == QProcess::NormalExit && exitCode == 0 && job.error.isEmpty();
    if (!ok && job.text.trimmed().isEmpty()) {
        QString err = job.error;
        if (err.isEmpty()) err = QString::fromUtf8(job.stderrTail).trimmed().section(QLatin1Char('\n'), -3);
        if (err.isEmpty()) err = tr("%1 exited with code %2").arg(name(job.provider)).arg(exitCode);
        LlmLog::write(name(job.provider), QStringLiteral("error [%1] after %2s: %3")
                                              .arg(tag).arg(secs, 0, 'f', 1).arg(err));
        emit failed(tag, err);
        return;
    }

    LlmLog::write(name(job.provider),
                  QStringLiteral("response [%1] %2 chars in %3s%4")
                      .arg(tag).arg(job.text.size()).arg(secs, 0, 'f', 1)
                      .arg(job.costUsd >= 0 ? QStringLiteral(", cost $%1").arg(job.costUsd, 0, 'f', 4)
                                            : QString()));
    emit finished(tag, job.text);
}

} // namespace Kayte::Llm
