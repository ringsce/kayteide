#ifndef CPPSYNTAXHIGHLIGHTER_H
#define CPPSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QSet>
#include <QHash> // Although not strictly used in this example, often useful for rule storage

class QTextDocument; // Forward declaration

class CppSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit CppSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat preprocessorFormat; // For #include, #define etc.
    QTextCharFormat numberFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat booleanFormat; // For true/false
    QTextCharFormat nullptrFormat; // For nullptr

    // For multi-line comments
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    void setupRules();
    void setupFormats();
};

#endif // CPPSYNTAXHIGHLIGHTER_H