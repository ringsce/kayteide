#ifndef VBSYNTAXHIGHLIGHTER_H
#define VBSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QHash>
#include <QSet>
#include <QRegularExpression> // <--- ADD THIS LINE

// Forward declaration (QTextDocument is typically fine as a forward declaration
// if only used as a pointer/reference in the header)
class QTextDocument;

class VBSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit VBSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern; // Now QRegularExpression is fully defined
        QTextCharFormat format;
    };

    // Lists of keywords, types, functions, etc., for efficient lookup
    QSet<QString> keywords;
    QSet<QString> types;
    QSet<QString> directives;
    QSet<QString> functions;
    QSet<QString> operators;

    // Formats for different token types
    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat directiveFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat identifierFormat;
    QTextCharFormat preprocessorFormat;

    // Helper functions for initialization
    void setupFormats();
    void setupKeywords();
};

#endif // VBSYNTAXHIGHLIGHTER_H