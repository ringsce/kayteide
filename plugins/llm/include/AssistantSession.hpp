#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Kayte::Llm {

struct ChatMessage
{
    QString   role;      // "user" | "assistant"
    QString   content;
    QDateTime at;
};

// A conversation with the assistant, stored as JSON in
//   <AppData>/llm-sessions/<id>.json
// The id is a 12-hex-digit hash, so a session can be reopened with
//   KayteIDE --resume <id>
// (any unambiguous prefix of 4+ characters works, like a git commit hash).
class AssistantSession
{
public:
    static AssistantSession create(const QString &projectRoot, const QString &model);
    static bool load(const QString &idOrPrefix, AssistantSession *out, QString *error);
    static QList<AssistantSession> recent(int limit = 15);   // newest first, no messages
    static QString storageDir();

    bool    save() const;
    bool    isValid() const { return !id.isEmpty(); }
    QString title() const;   // first user prompt, shortened

    QString              id;
    QString              projectRoot;
    QString              model;
    QDateTime            created;
    QDateTime            updated;
    QVector<ChatMessage> messages;
    QStringList          suggestions;   // prompts the LLM proposed to learn next
};

} // namespace Kayte::Llm
