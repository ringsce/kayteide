#pragma once
// UiXmlDocument.h – libreparser
// Reads a "window" document (see ui/ui.dtd), builds a generic element tree,
// and validates it against the ui.dtd grammar for debugging purposes.

#include <QString>
#include <QList>
#include <QPair>

struct XmlDiagnostic
{
    enum class Severity { Warning, Error };

    Severity severity;
    QString  message;
    qint64   line   { 0 };
    qint64   column { 0 };
};

struct XmlNode
{
    QString                         name;
    QList<QPair<QString, QString>>  attributes;
    QString                         text;
    QList<XmlNode>                  children;
    qint64                          line   { 0 };
    qint64                          column { 0 };

    QString attribute(const QString &key) const;
};

class UiXmlDocument
{
public:
    // Parses xmlSource and validates it against the ui.dtd grammar.
    // Returns true when parsing succeeded with no Error-level diagnostics.
    bool parse(const QString &xmlSource, QList<XmlDiagnostic> &diagnostics);

    const XmlNode &root() const { return m_root; }
    bool isValid() const { return m_valid; }

private:
    void validateWindow(const XmlNode &window, QList<XmlDiagnostic> &diagnostics) const;

    XmlNode m_root;
    bool    m_valid { false };
};
