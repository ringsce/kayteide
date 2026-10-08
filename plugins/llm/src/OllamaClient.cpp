#include "OllamaClient.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QEventLoop>
#include <QProcessEnvironment>
#include <QTimer>
#include <QCoreApplication>
#include <QThread>

#include <algorithm>

#ifdef Q_OS_UNIX
#  include <signal.h>
#  include <unistd.h>
#endif

namespace Kayte::Llm {

OllamaClient::OllamaClient(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
}

namespace {
// How long Ollama keeps a model in RAM after the last request. Short, because
// a 3B model pins ~2–3 GB; reloading it takes only a couple of seconds.
const char *kKeepAlive = "2m";
}

OllamaClient::~OllamaClient()
{
    // Ollama runs only while the IDE is open: the server we launched goes
    // away with us, and so does one that was already running when we started.
    if (m_server && m_server->state() != QProcess::NotRunning) {
        m_server->terminate();   // the watchdog shell stops `ollama serve`
        if (!m_server->waitForFinished(5000))
            m_server->kill();
        m_server->waitForFinished(1000);
    } else if (!m_server) {
        stopLocalServer();
    }
}

bool OllamaClient::isLocal() const
{
    const QString host = m_baseUrl.host();
    return host == QLatin1String("127.0.0.1") || host == QLatin1String("localhost")
        || host == QLatin1String("::1");
}

namespace {
QString runTool(const QString &program, const QStringList &args)
{
    QProcess p;
    p.start(program, args);
    if (!p.waitForFinished(2000)) { p.kill(); p.waitForFinished(500); return {}; }
    return QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
}
} // namespace

// Stops an Ollama server on our (local) port that we did not launch: quits
// Ollama.app when it owns the server, otherwise terminates `ollama serve`.
// Only processes named "ollama" are touched.
void OllamaClient::stopLocalServer()
{
#ifdef Q_OS_UNIX
    if (!isLocal()) return;
    const QString port = QString::number(m_baseUrl.port(11434));
    const QStringList pids = runTool(QStringLiteral("/usr/sbin/lsof"),
        { QStringLiteral("-nP"), QStringLiteral("-iTCP:") + port,
          QStringLiteral("-sTCP:LISTEN"), QStringLiteral("-t") })
        .split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    QList<pid_t> stopping;
    for (const QString &pidText : pids) {
        const pid_t pid = pidText.trimmed().toInt();
        if (pid <= 1 || pid == getpid()) continue;
        const QString comm = runTool(QStringLiteral("/bin/ps"),
                                     { QStringLiteral("-o"), QStringLiteral("comm="),
                                       QStringLiteral("-p"), QString::number(pid) });
        if (!comm.contains(QLatin1String("ollama"), Qt::CaseInsensitive)) continue;

        // Ollama.app runs the server as its child and would start another
        // one, so quit the app itself.
        const pid_t parent = runTool(QStringLiteral("/bin/ps"),
                                     { QStringLiteral("-o"), QStringLiteral("ppid="),
                                       QStringLiteral("-p"), QString::number(pid) }).toInt();
        if (parent > 1) {
            const QString parentComm = runTool(QStringLiteral("/bin/ps"),
                                               { QStringLiteral("-o"), QStringLiteral("comm="),
                                                 QStringLiteral("-p"), QString::number(parent) });
            if (parentComm.contains(QLatin1String("Ollama.app/"))) {
                ::kill(parent, SIGTERM);
                stopping << parent;
            }
        }
        ::kill(pid, SIGTERM);
        stopping << pid;
    }

    // Give them a moment to exit cleanly, then make sure.
    for (int i = 0; i < 30 && !stopping.isEmpty(); ++i) {
        stopping.erase(std::remove_if(stopping.begin(), stopping.end(),
                                      [](pid_t p) { return ::kill(p, 0) != 0; }),
                       stopping.end());
        if (!stopping.isEmpty()) QThread::msleep(100);
    }
    for (pid_t p : stopping) ::kill(p, SIGKILL);
#endif
}

// ─── Server availability ─────────────────────────────────────────────────────

void OllamaClient::ensureServer()
{
    ping(m_launchedServer ? 1 : 20);
}

void OllamaClient::ping(int attemptsLeft)
{
    QNetworkRequest req(m_baseUrl.resolved(QUrl(QStringLiteral("/api/tags"))));
    req.setTransferTimeout(1500);
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, attemptsLeft] {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit serverReady(true, QString());
            return;
        }

        // Not reachable: start `ollama serve` once, then poll for ~10 s.
        if (!m_launchedServer) {
            m_launchedServer = true;
            QString exe = QStandardPaths::findExecutable(QStringLiteral("ollama"));
            if (exe.isEmpty())
                exe = QStandardPaths::findExecutable(QStringLiteral("ollama"),
                        { QStringLiteral("/usr/local/bin"), QStringLiteral("/opt/homebrew/bin") });
            // Owned (not detached), so the destructor can stop it on exit.
            if (!exe.isEmpty()) {
                m_server = new QProcess(this);
                // Leaner server: 8-bit KV cache (needs flash attention) halves
                // the context's memory; one model and one request at a time.
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.insert(QStringLiteral("OLLAMA_FLASH_ATTENTION"), QStringLiteral("1"));
                env.insert(QStringLiteral("OLLAMA_KV_CACHE_TYPE"), QStringLiteral("q8_0"));
                env.insert(QStringLiteral("OLLAMA_MAX_LOADED_MODELS"), QStringLiteral("1"));
                env.insert(QStringLiteral("OLLAMA_NUM_PARALLEL"), QStringLiteral("1"));
                env.insert(QStringLiteral("OLLAMA_KEEP_ALIVE"), QString::fromLatin1(kKeepAlive));
                m_server->setProcessEnvironment(env);
#ifdef Q_OS_UNIX
                // A watchdog shell runs `ollama serve` and stops it when the
                // IDE exits — normally (SIGTERM from our destructor) or not
                // (crash, force quit), so the server never outlives the IDE.
                m_server->setProgram(QStringLiteral("/bin/sh"));
                m_server->setArguments({
                    QStringLiteral("-c"),
                    QStringLiteral(
                        "\"$2\" serve & srv=$!\n"
                        "trap 'kill $srv 2>/dev/null; wait $srv; exit 0' TERM INT HUP\n"
                        "while kill -0 \"$1\" 2>/dev/null && kill -0 $srv 2>/dev/null; do\n"
                        "  sleep 1 & wait $!\n"
                        "done\n"
                        "kill $srv 2>/dev/null; wait $srv\n"),
                    QStringLiteral("kayteide-ollama"),
                    QString::number(QCoreApplication::applicationPid()),
                    exe });
#else
                m_server->setProgram(exe);
                m_server->setArguments({ QStringLiteral("serve") });
#endif
                m_server->setStandardOutputFile(QProcess::nullDevice());
                m_server->setStandardErrorFile(QProcess::nullDevice());
                // No waitForStarted(): never block the GUI thread.
                connect(m_server, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
                    if (e == QProcess::FailedToStart)
                        emit serverReady(false, tr("Ollama could not be started."));
                });
                m_server->start();
            } else {
                emit serverReady(false, tr("Ollama is not running and could not be "
                                           "started. Install it from https://ollama.com"));
                return;
            }
        }
        if (m_server && m_server->state() == QProcess::NotRunning && m_server->error() == QProcess::FailedToStart)
            return;   // already reported
        if (attemptsLeft <= 1) {
            emit serverReady(false, tr("Ollama did not answer at %1 (%2)")
                                        .arg(m_baseUrl.toString(), reply->errorString()));
            return;
        }
        QTimer::singleShot(500, this, [this, attemptsLeft] { ping(attemptsLeft - 1); });
    });
}

void OllamaClient::listModels()
{
    QNetworkReply *reply = m_net->get(
        QNetworkRequest(m_baseUrl.resolved(QUrl(QStringLiteral("/api/tags")))));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        QStringList names;
        const auto models = QJsonDocument::fromJson(reply->readAll())
                                .object().value(QStringLiteral("models")).toArray();
        for (const auto &m : models)
            names << m.toObject().value(QStringLiteral("name")).toString();
        emit modelsListed(names);
    });
}

// ─── Streaming chat ──────────────────────────────────────────────────────────

void OllamaClient::chat(const QString &tag, const QString &model,
                        const QJsonArray &messages, int maxTokens, int numCtx)
{
    abort(tag);
    m_usedModels.insert(model);

    QJsonObject body {
        { QStringLiteral("model"),      model },
        { QStringLiteral("messages"),   messages },
        { QStringLiteral("stream"),     true },
        { QStringLiteral("keep_alive"), QString::fromLatin1(kKeepAlive) },
    };
    QJsonObject options { { QStringLiteral("num_ctx"), numCtx } };
    if (maxTokens > 0)
        options.insert(QStringLiteral("num_predict"), maxTokens);
    body.insert(QStringLiteral("options"), options);

    QNetworkRequest req(m_baseUrl.resolved(QUrl(QStringLiteral("/api/chat"))));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_net->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    m_replies.insert(tag, reply);
    m_buffers.insert(tag, {});
    m_texts.insert(tag, {});

    // Ollama streams one JSON object per line.
    connect(reply, &QNetworkReply::readyRead, this, [this, tag, reply] {
        QByteArray &buf = m_buffers[tag];
        buf += reply->readAll();
        int nl;
        while ((nl = buf.indexOf('\n')) >= 0) {
            const QJsonObject obj = QJsonDocument::fromJson(buf.left(nl)).object();
            buf.remove(0, nl + 1);
            if (obj.contains(QStringLiteral("error"))) {
                emit failed(tag, obj.value(QStringLiteral("error")).toString());
                continue;
            }
            const QString piece = obj.value(QStringLiteral("message")).toObject()
                                     .value(QStringLiteral("content")).toString();
            if (!piece.isEmpty()) {
                m_texts[tag] += piece;
                emit token(tag, piece);
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, tag, reply] {
        reply->deleteLater();
        if (m_replies.value(tag) != reply) return;   // superseded / aborted
        m_replies.remove(tag);
        m_buffers.remove(tag);
        const QString text = m_texts.take(tag);

        if (reply->error() != QNetworkReply::NoError &&
            reply->error() != QNetworkReply::OperationCanceledError) {
            QString msg = reply->errorString();
            const QJsonObject err = QJsonDocument::fromJson(reply->readAll()).object();
            if (err.contains(QStringLiteral("error")))
                msg = err.value(QStringLiteral("error")).toString();
            emit failed(tag, msg);
            return;
        }
        emit finished(tag, text);
    });
}

// keep_alive 0 tells Ollama to drop the model from memory immediately.
void OllamaClient::unload(const QString &model)
{
    if (model.isEmpty()) return;
    m_usedModels.remove(model);
    QNetworkRequest req(m_baseUrl.resolved(QUrl(QStringLiteral("/api/generate"))));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setTransferTimeout(3000);
    QNetworkReply *reply = m_net->post(req, QJsonDocument(QJsonObject {
        { QStringLiteral("model"), model }, { QStringLiteral("keep_alive"), 0 } })
        .toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

// Used at shutdown, when the event loop is about to stop: wait (briefly) for
// the unload requests to reach the server.
void OllamaClient::unloadAll()
{
    if (m_usedModels.isEmpty()) return;
    const QSet<QString> models = std::exchange(m_usedModels, {});
    QEventLoop loop;
    int pending = 0;
    for (const QString &m : models) {
        QNetworkRequest req(m_baseUrl.resolved(QUrl(QStringLiteral("/api/generate"))));
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        req.setTransferTimeout(1500);
        QNetworkReply *reply = m_net->post(req, QJsonDocument(QJsonObject {
            { QStringLiteral("model"), m }, { QStringLiteral("keep_alive"), 0 } })
            .toJson(QJsonDocument::Compact));
        ++pending;
        connect(reply, &QNetworkReply::finished, &loop, [&loop, &pending, reply] {
            reply->deleteLater();
            if (--pending == 0) loop.quit();
        });
    }
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();
}

void OllamaClient::abort(const QString &tag)
{
    if (QNetworkReply *reply = m_replies.take(tag)) {
        // Silent: the caller already holds the streamed tokens.
        m_buffers.remove(tag);
        m_texts.remove(tag);
        reply->abort();
    }
}

} // namespace Kayte::Llm
