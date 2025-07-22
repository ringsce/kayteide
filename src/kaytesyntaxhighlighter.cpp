// kaytesyntaxhighlighter.cpp
#include "kaytesyntaxhighlighter.h" // Include your header
#include <QTextDocument> // QSyntaxHighlighter works with QTextDocument
#include <QRegularExpression> // For pattern matching
#include <QBrush> // For text color
#include <QFont> // For font styles
#include <algorithm> // For std::sort
#include <utility>   // <--- ADD THIS LINE FOR std::as_const

// Constructor
KayteSyntaxHighlighter::KayteSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Define text formats (colors, fonts)
    // Keyword format (Pink/Red)
    keywordFormat.setForeground(QBrush(QColor("#f92672")));
    keywordFormat.setFontWeight(QFont::Bold);

    // Operator format (Cyan)
    operatorFormat.setForeground(QBrush(QColor("#66d9ef")));

    // String format (Yellow)
    stringFormat.setForeground(QBrush(QColor("#e6db74")));

    // Number format (Purple)
    numberFormat.setForeground(QBrush(QColor("#ae81ff")));

    // Boolean literal format (Purple, bold)
    booleanLiteralFormat.setForeground(QBrush(QColor("#ae81ff")));
    booleanLiteralFormat.setFontWeight(QFont::Bold);

    // Comment format (Gray, italic)
    commentFormat.setForeground(QBrush(QColor("#75715e")));
    commentFormat.setFontItalic(true);

    // Identifier format (Default white, but explicitly set if needed for clarity)
    identifierFormat.setForeground(QBrush(QColor("#f8f8f2")));

    // Directive format (Orange, bold)
    directiveFormat.setForeground(QBrush(QColor("#fd971f")));
    directiveFormat.setFontWeight(QFont::Bold);

    // Error format (Red background, red text)
    errorFormat.setForeground(QBrush(QColor("#ff0000")));
    errorFormat.setBackground(QBrush(QColor("#330000")));


    // Populate highlighting rules
    HighlightingRule rule;

    // 1. Keywords (case-insensitive)
    QStringList keywordPatterns;
    keywordPatterns << "\\bPRINT\\b" << "\\bSHOW\\b" << "\\bIF\\b" << "\\bTHEN\\b" << "\\bEND\\b"
                    << "\\bSUB\\b" << "\\bFUNCTION\\b" << "\\bDIM\\b" << "\\bAS\\b"
                    << "\\bELSE\\b" << "\\bELSEIF\\b" << "\\bENDIF\\b" << "\\bSELECT\\b"
                    << "\\bCASE\\b" << "\\bEND\\s+SELECT\\b" // "END SELECT" is a multi-word keyword
                    << "\\bWHILE\\b" << "\\bWEND\\b" << "\\bFOR\\b" << "\\bNEXT\\b"
                    << "\\bTO\\b" << "\\bSTEP\\b" << "\\bREDIM\\b" << "\\bPRESERVE\\b"
                    << "\\bCALL\\b" << "\\bGOTO\\b" << "\\bGOSUB\\b" << "\\bRETURN\\b"
                    << "\\bINPUT\\b" << "\\bMSGBOX\\b" << "\\bFORM\\b" << "\\bEND\\s+FORM\\b" // "END FORM"
                    << "\\bHIDE\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // 2. Boolean Literals (case-insensitive)
    QStringList booleanPatterns;
    booleanPatterns << "\\bTRUE\\b" << "\\bFALSE\\b";
    for (const QString &pattern : booleanPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = booleanLiteralFormat;
        highlightingRules.append(rule);
    }

    // 3. Numbers (integers and decimals)
    rule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // 4. Strings (quoted text)
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    // 5. Single-line comments (starting with ' or REM)
    rule.pattern = QRegularExpression("'[^\n]*"); // Matches ' to the end of the line
    rule.format = commentFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\bREM\\b.*", QRegularExpression::CaseInsensitiveOption); // Matches REM to end of line
    rule.format = commentFormat;
    highlightingRules.append(rule);

    // 6. Directives (e.g., Option Explicit On/Off)
    rule.pattern = QRegularExpression("\\bOPTION\\s+EXPLICIT\\s+(?:ON|OFF)\\b", QRegularExpression::CaseInsensitiveOption);
    rule.format = directiveFormat;
    highlightingRules.append(rule);

    // 7. Operators (single and multi-character)
    QStringList operatorPatterns;
    // Escape special regex characters like +, *, /, (, ), ., :
    operatorPatterns << "<=" << ">=" << "<>" << "=" << "\\+" << "-" << "\\*" << "/"
                     << "<" << ">" << "&" << "\\(" << "\\)" << "," << "\\." << ":";
    // Add Kayte-specific word operators (AND, OR, NOT, IS)
    operatorPatterns << "\\bAND\\b" << "\\bOR\\b" << "\\bNOT\\b" << "\\bIS\\b";


    // Sort operators by length descending to ensure longest match first
    std::sort(operatorPatterns.begin(), operatorPatterns.end(),
              [](const QString &a, const QString &b){ return a.length() > b.length(); });

    for (const QString &pattern : operatorPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = operatorFormat;
        highlightingRules.append(rule);
    }
}

// highlightBlock is called by QSyntaxHighlighter for each text block (line)
void KayteSyntaxHighlighter::highlightBlock(const QString &text)
{
    // Apply all highlighting rules
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) { // <--- CHANGED qAsConst to std::as_const
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Set block state to 0 (No special multi-line state)
    // Only use if you don't have multi-line constructs or you've handled them.
    setCurrentBlockState(0);
}