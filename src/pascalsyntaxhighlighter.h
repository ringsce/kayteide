// PascalSyntaxHighlighter.h
#ifndef PASCALSYNTAXHIGHLIGHTER_H
#define PASCALSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class PascalSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit PascalSyntaxHighlighter(QTextDocument *parent = nullptr);

protected: // Changed from 'private' to 'protected'
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    // --- CHANGE THESE FROM 'private' to 'protected' ---
    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;      // <--- Make this protected
    QTextCharFormat numberFormat;
    QTextCharFormat preprocessorFormat;
    // ---------------------------------------------------

    // For multi-line comments
    // These expressions don't need to be protected unless derived classes directly
    // manipulate them for their own multi-line logic.
    // For now, keep them private, as `highlightBlock` will use them.
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    void highlightBlock(const QString &text) override; // Keep this protected for derived classes to call

private: // Keep these private, as they are internal to PascalSyntaxHighlighter's setup process
    // and not directly manipulated by derived classes in this design.
    void setupFormats();
    void setupRules();
};

#endif // PASCALSYNTAXHIGHLIGHTER_H