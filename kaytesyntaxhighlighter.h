// KayteSyntaxHighlighter.h
#ifndef KAYTE_SYNTAX_HIGHLIGHTER_H
#define KAYTE_SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector> // Include QVector for HighlightingRule list

// Class to perform syntax highlighting for Kayte language
class KayteSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit KayteSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    // Structure to hold a highlighting rule
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules; // Use QVector for rules

    // Declare all QTextCharFormat members here
    QTextCharFormat keywordFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat booleanLiteralFormat;
    QTextCharFormat directiveFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat identifierFormat; // Declare identifierFormat
    QTextCharFormat errorFormat;      // Declare errorFormat - THIS FIXES THE UNDECLARED IDENTIFIER ERROR
};

#endif // KAYTE_SYNTAX_HIGHLIGHTER_H