#include "UiXmlDocument.h"

#include <QXmlStreamReader>
#include <algorithm>

QString XmlNode::attribute(const QString &key) const
{
    for (const auto &pair : attributes)
        if (pair.first == key)
            return pair.second;
    return {};
}

// Recursive-descent read of the element the reader is currently positioned
// on (a StartElement). Returns the fully-built node, children included.
static XmlNode parseElement(QXmlStreamReader &xml)
{
    XmlNode node;
    node.name   = xml.name().toString();
    node.line   = xml.lineNumber();
    node.column = xml.columnNumber();

    for (const auto &attr : xml.attributes())
        node.attributes.append({attr.name().toString(), attr.value().toString()});

    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartElement:
            node.children.append(parseElement(xml));
            break;
        case QXmlStreamReader::Characters:
            if (!xml.isWhitespace())
                node.text += xml.text().toString();
            break;
        case QXmlStreamReader::EndElement:
            return node;
        case QXmlStreamReader::Invalid:
            return node;
        default:
            break;
        }
    }
    return node;
}

bool UiXmlDocument::parse(const QString &xmlSource, QList<XmlDiagnostic> &diagnostics)
{
    diagnostics.clear();
    m_valid = false;
    m_root  = XmlNode{};

    QXmlStreamReader xml(xmlSource);

    while (!xml.atEnd() && xml.tokenType() != QXmlStreamReader::StartElement)
        xml.readNext();

    if (xml.tokenType() != QXmlStreamReader::StartElement) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
                             QStringLiteral("No root element found"), 0, 0});
        return false;
    }

    m_root = parseElement(xml);

    if (xml.hasError()) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
                             xml.errorString(), xml.lineNumber(), xml.columnNumber()});
        return false;
    }

    if (m_root.name != QStringLiteral("window")) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("Root element must be <window>, found <%1>").arg(m_root.name),
            m_root.line, m_root.column});
        return false;
    }

    validateWindow(m_root, diagnostics);

    m_valid = std::none_of(diagnostics.cbegin(), diagnostics.cend(),
                            [](const XmlDiagnostic &d) {
                                return d.severity == XmlDiagnostic::Severity::Error;
                            });
    return m_valid;
}

void UiXmlDocument::validateWindow(const XmlNode &window, QList<XmlDiagnostic> &diagnostics) const
{
    if (window.attribute(QStringLiteral("id")).isEmpty()) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("<window> is missing required attribute \"id\""),
            window.line, window.column});
    }

    const XmlNode *title = nullptr, *content = nullptr, *actions = nullptr;
    int titleCount = 0, contentCount = 0, actionsCount = 0;

    for (const XmlNode &child : window.children) {
        if (child.name == QStringLiteral("title")) {
            ++titleCount;
            title = &child;
        } else if (child.name == QStringLiteral("content")) {
            ++contentCount;
            content = &child;
        } else if (child.name == QStringLiteral("actions")) {
            ++actionsCount;
            actions = &child;
        } else {
            diagnostics.append({XmlDiagnostic::Severity::Warning,
                QStringLiteral("Unexpected element <%1> inside <window>").arg(child.name),
                child.line, child.column});
        }
    }

    if (titleCount == 0)
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("<window> requires exactly one <title>"), window.line, window.column});
    else if (titleCount > 1)
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("<window> must not contain more than one <title>"), window.line, window.column});
    else if (title->text.trimmed().isEmpty())
        diagnostics.append({XmlDiagnostic::Severity::Warning,
            QStringLiteral("<title> is empty"), title->line, title->column});

    if (contentCount != 1) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("<window> requires exactly one <content>"), window.line, window.column});
    } else if (content->children.isEmpty()) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("<content> requires at least one <text>"), content->line, content->column});
    } else {
        for (const XmlNode &c : content->children) {
            if (c.name != QStringLiteral("text"))
                diagnostics.append({XmlDiagnostic::Severity::Warning,
                    QStringLiteral("Unexpected element <%1> inside <content>").arg(c.name),
                    c.line, c.column});
        }
    }

    if (actionsCount != 1) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("<window> requires exactly one <actions>"), window.line, window.column});
    } else if (actions->children.isEmpty()) {
        diagnostics.append({XmlDiagnostic::Severity::Error,
            QStringLiteral("<actions> requires at least one <button>"), actions->line, actions->column});
    } else {
        for (const XmlNode &btn : actions->children) {
            if (btn.name != QStringLiteral("button")) {
                diagnostics.append({XmlDiagnostic::Severity::Warning,
                    QStringLiteral("Unexpected element <%1> inside <actions>").arg(btn.name),
                    btn.line, btn.column});
                continue;
            }
            if (btn.attribute(QStringLiteral("id")).isEmpty())
                diagnostics.append({XmlDiagnostic::Severity::Error,
                    QStringLiteral("<button> is missing required attribute \"id\""), btn.line, btn.column});
            if (btn.attribute(QStringLiteral("onclick")).isEmpty())
                diagnostics.append({XmlDiagnostic::Severity::Warning,
                    QStringLiteral("<button> has no \"onclick\" handler"), btn.line, btn.column});
        }
    }
}
