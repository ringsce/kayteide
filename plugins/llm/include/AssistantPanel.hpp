#pragma once

#include "AssistantSession.hpp"
#include "CliAgentClient.hpp"
#include "CodeIndex.hpp"
#include "OllamaClient.hpp"

#include <QPair>
#include <QWidget>

#include <functional>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;
class QTimer;
class QToolButton;
class QVBoxLayout;

namespace Kayte::Llm {

// Chat panel for the local LLM. Learns the open project (CodeIndex), answers
// with the relevant code as context, proposes prompts to learn from, and keeps
// every conversation in a resumable, hash-named AssistantSession.
class AssistantPanel : public QWidget
{
    Q_OBJECT
public:
    // Returns { absolute path, full text } of the file open in the editor.
    using CurrentFileProvider = std::function<QPair<QString, QString>()>;

    explicit AssistantPanel(QWidget *parent = nullptr);
    ~AssistantPanel() override;

    void setProjectRoot(const QString &root);
    void setCurrentFileProvider(CurrentFileProvider provider) { m_currentFile = std::move(provider); }

    bool    resumeSession(const QString &idOrPrefix, QString *error = nullptr);
    void    newSession();
    void    saveSession();
    QString sessionId() const     { return m_session.id; }
    QString sessionProject() const { return m_session.projectRoot; }
    bool    hasConversation() const { return !m_session.messages.isEmpty(); }

public slots:
    void learnProject();
    void ask(const QString &prompt);

signals:
    void statusMessage(const QString &message);
    void openFileRequested(const QString &absolutePath, int line);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void buildUi();
    void onServerReady(bool ok, const QString &error);
    void showOllamaStatus();
    void onToken(const QString &tag, const QString &text);
    void onFinished(const QString &tag, const QString &text);
    void onFailed(const QString &tag, const QString &error);
    void onIndexReady();
    void sendOrStop();
    void finishAnswer(const QString &text, const QString &note = QString());
    void requestSuggestions(bool followUp);
    void setSuggestions(const QStringList &prompts);
    void scheduleRender();
    void render();
    void updateSessionLabel();
    void populateHistoryMenu();
    void setBusy(bool busy);
    // Model keys: "ollama/<name>", "claude/<model id>", "codex/<model or empty>".
    QString currentKey() const;
    QString currentLabel() const;
    static QString providerOf(const QString &key);
    static QString modelOf(const QString &key);
    static CliAgentClient::Provider cliProvider(const QString &key);
    bool    providerReady() const;
    void    rebuildModelList();
    void    onModelChanged(bool userInitiated);
    void    onLoginStatus(CliAgentClient::Provider provider, bool installed,
                          bool loggedIn, const QString &detail);
    void    promptSignIn(CliAgentClient::Provider provider, bool installed);
    void    abortTag(const QString &tag);
    QString systemPrompt() const;

    OllamaClient        m_client;
    CliAgentClient      m_cli;
    QHash<int, bool>    m_cliSignedIn;          // CliAgentClient::Provider → signed in
    QStringList         m_ollamaModels;
    QTimer             *m_loginPoll   { nullptr };
    int                 m_pollProvider { -1 };
    int                 m_pollCount   { 0 };
    qint64              m_askStarted  { 0 };
    bool                m_modelChosen { false };   // user picked / session resumed
    QString             m_activeKey;               // model currently selected
    int                 m_ollamaCtx   { 4096 };    // context window last used locally
    QString             m_ollamaError;
    CodeIndex           m_index;
    AssistantSession    m_session;
    CurrentFileProvider m_currentFile;
    QString             m_pendingRoot;      // root to learn once the index is idle
    QString             m_streaming;        // answer being received
    QString             m_lastSources;      // markdown list of context chunks used
    bool                m_busy       { false };
    bool                m_serverOk   { false };

    QComboBox      *m_model       { nullptr };
    QToolButton    *m_learnBtn    { nullptr };
    QToolButton    *m_historyBtn  { nullptr };
    QLabel         *m_sessionLbl  { nullptr };
    QLabel         *m_statusLbl   { nullptr };
    QTextBrowser   *m_transcript  { nullptr };
    QWidget        *m_suggestBox  { nullptr };
    QVBoxLayout    *m_suggestLay  { nullptr };
    QPlainTextEdit *m_input       { nullptr };
    QPushButton    *m_sendBtn     { nullptr };
    QTimer         *m_renderTimer { nullptr };
};

} // namespace Kayte::Llm
