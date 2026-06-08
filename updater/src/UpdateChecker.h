#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QString>
#include <QDateTime>

struct CommitInfo {
    QString sha;
    QString shortSha;
    QString message;
    QString author;
    QDateTime timestamp;
    QString url;
};

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker() override = default;

    void setRepoApiUrl(const QString &url);
    void setCheckInterval(int minutes);
    void setCurrentSha(const QString &sha);

    QString currentSha() const { return m_currentSha; }
    QString latestSha()  const { return m_latestSha; }
    bool    hasUpdate()  const { return m_hasUpdate; }

public slots:
    void startAutoCheck();
    void stopAutoCheck();
    void checkNow();

signals:
    void updateAvailable(const CommitInfo &commit);
    void alreadyUpToDate();
    void checkFailed(const QString &error);
    void checkStarted();

private slots:
    void onCommitReplyFinished(QNetworkReply *reply);

private:
    CommitInfo parseCommit(const QByteArray &json);

    QNetworkAccessManager *m_nam       = nullptr;
    QTimer                *m_timer     = nullptr;
    QString                m_apiUrl    = "https://api.github.com/repos/ringsce/kayteide";
    QString                m_branch    = "main";
    QString                m_currentSha;
    QString                m_latestSha;
    bool                   m_hasUpdate = false;
};
