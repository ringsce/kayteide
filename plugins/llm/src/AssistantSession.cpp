#include "AssistantSession.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

namespace Kayte::Llm {

namespace {

QJsonObject toJson(const AssistantSession &s)
{
    QJsonArray msgs;
    for (const ChatMessage &m : s.messages)
        msgs.append(QJsonObject {
            { QStringLiteral("role"),    m.role },
            { QStringLiteral("content"), m.content },
            { QStringLiteral("at"),      m.at.toString(Qt::ISODate) },
        });
    return {
        { QStringLiteral("id"),          s.id },
        { QStringLiteral("projectRoot"), s.projectRoot },
        { QStringLiteral("model"),       s.model },
        { QStringLiteral("created"),     s.created.toString(Qt::ISODate) },
        { QStringLiteral("updated"),     s.updated.toString(Qt::ISODate) },
        { QStringLiteral("messages"),    msgs },
        { QStringLiteral("suggestions"), QJsonArray::fromStringList(s.suggestions) },
    };
}

bool readFile(const QString &path, AssistantSession *out, bool withMessages)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    if (o.isEmpty()) return false;

    out->id          = o.value(QStringLiteral("id")).toString();
    out->projectRoot = o.value(QStringLiteral("projectRoot")).toString();
    out->model       = o.value(QStringLiteral("model")).toString();
    out->created     = QDateTime::fromString(o.value(QStringLiteral("created")).toString(), Qt::ISODate);
    out->updated     = QDateTime::fromString(o.value(QStringLiteral("updated")).toString(), Qt::ISODate);
    out->messages.clear();
    out->suggestions.clear();
    for (const auto &v : o.value(QStringLiteral("suggestions")).toArray())
        out->suggestions << v.toString();

    const QJsonArray msgs = o.value(QStringLiteral("messages")).toArray();
    // recent() only needs the first prompt for the title.
    for (const auto &v : msgs) {
        const QJsonObject m = v.toObject();
        out->messages.append({ m.value(QStringLiteral("role")).toString(),
                               m.value(QStringLiteral("content")).toString(),
                               QDateTime::fromString(m.value(QStringLiteral("at")).toString(),
                                                     Qt::ISODate) });
        if (!withMessages && out->messages.size() >= 1) break;
    }
    return !out->id.isEmpty();
}

} // namespace

QString AssistantSession::storageDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + QStringLiteral("/llm-sessions");
    QDir().mkpath(dir);
    return dir;
}

AssistantSession AssistantSession::create(const QString &projectRoot, const QString &model)
{
    AssistantSession s;
    s.projectRoot = projectRoot;
    s.model       = model;
    s.created     = s.updated = QDateTime::currentDateTime();

    // Hash of (time, project, random) → short, stable, unique id.
    QCryptographicHash h(QCryptographicHash::Sha256);
    h.addData(s.created.toString(Qt::ISODateWithMs).toUtf8());
    h.addData(projectRoot.toUtf8());
    h.addData(QByteArray::number(QRandomGenerator::global()->generate64()));
    s.id = QString::fromLatin1(h.result().toHex().left(12));
    return s;
}

bool AssistantSession::load(const QString &idOrPrefix, AssistantSession *out, QString *error)
{
    const QString key = idOrPrefix.trimmed().toLower();
    if (key.size() < 4) {
        if (error) *error = QObject::tr("Session hash must have at least 4 characters.");
        return false;
    }
    const QStringList matches = QDir(storageDir())
        .entryList({ key + QStringLiteral("*.json") }, QDir::Files);
    if (matches.isEmpty()) {
        if (error) *error = QObject::tr("No assistant session matches '%1'.").arg(key);
        return false;
    }
    if (matches.size() > 1) {
        if (error) *error = QObject::tr("'%1' is ambiguous (%2 sessions match).")
                                .arg(key).arg(matches.size());
        return false;
    }
    if (!readFile(storageDir() + QLatin1Char('/') + matches.first(), out, true)) {
        if (error) *error = QObject::tr("Session file %1 is unreadable.").arg(matches.first());
        return false;
    }
    return true;
}

QList<AssistantSession> AssistantSession::recent(int limit)
{
    QList<AssistantSession> out;
    const QDir dir(storageDir());
    const auto files = dir.entryInfoList({ QStringLiteral("*.json") }, QDir::Files, QDir::Time);
    for (const QFileInfo &fi : files) {
        AssistantSession s;
        if (readFile(fi.absoluteFilePath(), &s, false)) out << s;
        if (out.size() >= limit) break;
    }
    return out;
}

bool AssistantSession::save() const
{
    if (id.isEmpty() || messages.isEmpty()) return false;   // don't litter empty sessions
    QSaveFile f(storageDir() + QLatin1Char('/') + id + QStringLiteral(".json"));
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(toJson(*this)).toJson(QJsonDocument::Indented));
    return f.commit();
}

QString AssistantSession::title() const
{
    for (const ChatMessage &m : messages) {
        if (m.role != QLatin1String("user")) continue;
        QString t = m.content.simplified();
        if (t.size() > 60) t = t.left(57) + QStringLiteral("…");
        return t;
    }
    return QObject::tr("(empty)");
}

} // namespace Kayte::Llm
