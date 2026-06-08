#include "UpdateChecker.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
{
    m_nam   = new QNetworkAccessManager(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(30 * 60 * 1000); // default: 30 min

    connect(m_timer, &QTimer::timeout, this, &UpdateChecker::checkNow);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onCommitReplyFinished);
}

void UpdateChecker::setRepoApiUrl(const QString &url) { m_apiUrl = url; }
void UpdateChecker::setCurrentSha(const QString &sha) { m_currentSha = sha; }

void UpdateChecker::setCheckInterval(int minutes)
{
    m_timer->setInterval(minutes * 60 * 1000);
}

void UpdateChecker::startAutoCheck()
{
    checkNow();
    m_timer->start();
}

void UpdateChecker::stopAutoCheck()
{
    m_timer->stop();
}

void UpdateChecker::checkNow()
{
    emit checkStarted();

    // Query the latest commit on the default branch via GitHub API
    QString endpoint = QString("%1/commits/%2").arg(m_apiUrl, m_branch);
    QNetworkRequest req{QUrl(endpoint)};
    req.setRawHeader("Accept", "application/vnd.github.v3+json");
    req.setRawHeader("User-Agent", "KayteIDE-AutoUpdater/1.0");

    m_nam->get(req);
}

void UpdateChecker::onCommitReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed(reply->errorString());
        return;
    }

    const QByteArray data = reply->readAll();
    CommitInfo commit = parseCommit(data);

    if (commit.sha.isEmpty()) {
        emit checkFailed("Failed to parse GitHub API response.");
        return;
    }

    m_latestSha = commit.sha;

    if (!m_currentSha.isEmpty() && m_currentSha == m_latestSha) {
        m_hasUpdate = false;
        emit alreadyUpToDate();
    } else {
        m_hasUpdate = true;
        m_currentSha = m_latestSha; // prevent repeat signals
        emit updateAvailable(commit);
    }
}

CommitInfo UpdateChecker::parseCommit(const QByteArray &json)
{
    CommitInfo info;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return info;

    QJsonObject root = doc.object();
    info.sha      = root["sha"].toString();
    info.shortSha = info.sha.left(7);
    info.url      = root["html_url"].toString();

    QJsonObject commitObj = root["commit"].toObject();
    info.message = commitObj["message"].toString().split('\n').first(); // first line only

    QJsonObject authorObj = commitObj["author"].toObject();
    info.author    = authorObj["name"].toString();
    info.timestamp = QDateTime::fromString(authorObj["date"].toString(), Qt::ISODate);

    return info;
}
