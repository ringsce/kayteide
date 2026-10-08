#ifndef MARKDOWNSYNTAXHIGHLIGHTER_H
#define MARKDOWNSYNTAXHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

// Highlights Markdown (CommonMark + GitHub tables/task lists): headings,
// emphasis, inline and fenced code, links and images, lists, block quotes
// and rules. Colours follow the editor palette, so they suit light and dark
// themes.
class MarkdownSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit MarkdownSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    enum BlockState { Normal = 0, InFence = 1 };

    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
        int group = 0;   // capture group to format (0 = whole match)
    };
    QVector<Rule> m_inlineRules;

    QTextCharFormat m_heading;
    QTextCharFormat m_code;
    QTextCharFormat m_fenceMarker;
    QTextCharFormat m_quote;
    QTextCharFormat m_listMarker;
    QTextCharFormat m_rule;
};

#endif // MARKDOWNSYNTAXHIGHLIGHTER_H
