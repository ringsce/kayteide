#pragma once

#include <QHash>
#include <QJsonArray>
#include <QSet>
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QProcess;

namespace Kayte::Llm {

// Minimal async client for a local Ollama server.
//   * ensureServer()  – pings /api/tags, launches `ollama serve` if needed
//
// The local server lives exactly as long as the IDE: the one we launch is
// tied to our process (it stops even if the IDE crashes), and on exit any
// local Ollama server — including one that was already running — is stopped.
//   * listModels()    – installed models
//   * chat()          – streaming /api/chat; requests are identified by a tag
//                       so several (e.g. "answer", "suggest") can coexist.
class OllamaClient : public QObject
{
    Q_OBJECT
public:
    explicit OllamaClient(QObject *parent = nullptr);
    ~OllamaClient() override;   // stops the local Ollama server

    void setBaseUrl(const QUrl &url) { m_baseUrl = url; }
    QUrl baseUrl() const { return m_baseUrl; }

    void ensureServer();
    void listModels();
    // numCtx: context window to load the model with. Keep it stable between
    // calls; a different value makes Ollama reload the model.
    void chat(const QString &tag, const QString &model, const QJsonArray &messages,
              int maxTokens = -1, int numCtx = 4096);
    void unload(const QString &model);   // free the model's RAM now
    void unloadAll();
    void abort(const QString &tag);
    bool isBusy(const QString &tag) const { return m_replies.contains(tag); }

signals:
    void serverReady(bool ok, const QString &error);
    void modelsListed(const QStringList &models);
    void token(const QString &tag, const QString &text);
    void finished(const QString &tag, const QString &fullText);
    void failed(const QString &tag, const QString &error);

private:
    void ping(int attemptsLeft);
    bool isLocal() const;
    void stopLocalServer();     // one we did not launch (Ollama.app, a leftover)

    QNetworkAccessManager         *m_net;
    QUrl                           m_baseUrl { QStringLiteral("http://127.0.0.1:11434") };
    QHash<QString, QNetworkReply*> m_replies;
    QHash<QString, QByteArray>     m_buffers;
    QHash<QString, QString>        m_texts;
    bool                           m_launchedServer { false };
    QProcess                      *m_server { nullptr };   // only set if we launched it
    QSet<QString>                  m_usedModels;           // to unload on exit
};

} // namespace Kayte::Llm
