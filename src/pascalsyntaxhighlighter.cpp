#include "pascalsyntaxhighlighter.h"
#include <utility> // Required for std::as_const

PascalSyntaxHighlighter::PascalSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
    setupRules();

    commentStartExpression = QRegularExpression("\\{\\s*"); // Match '{' potentially followed by spaces
    commentEndExpression = QRegularExpression("\\s*\\}");   // Match '}' potentially preceded by spaces
}

void PascalSyntaxHighlighter::setupFormats()
{
    // Keywords (e.g., BEGIN, END, VAR, FUNCTION, PROCEDURE)
    keywordFormat.setForeground(QColor(0, 0, 170)); // Dark Blue
    keywordFormat.setFontWeight(QFont::Bold);

    // Types (e.g., INTEGER, REAL, BOOLEAN, STRING)
    typeFormat.setForeground(QColor(0, 100, 0)); // Dark Green

    // Functions/Procedures (built-in or common Pascal ones)
    functionFormat.setForeground(QColor(150, 0, 150)); // Purple

    // Operators (e.g., :=, +, -, *, /, <, >, =, <=, >=, <>)
    operatorFormat.setForeground(QColor(100, 0, 0)); // Dark Red

    // Strings
    stringFormat.setForeground(QColor(160, 20, 20)); // Red
    stringFormat.setFontWeight(QFont::Normal);

    // Single-line Comments ({ ... } or // ... )
    commentFormat.setForeground(QColor(128, 128, 128)); // Gray
    commentFormat.setFontItalic(true);

    // Numbers
    numberFormat.setForeground(QColor(50, 50, 200)); // Medium Blue

    // Preprocessor directives (e.g., {$IFDEF}) - less common in pure Pascal, but good to include
    preprocessorFormat.setForeground(QColor(128, 0, 128)); // Dark Purple
}

void PascalSyntaxHighlighter::setupRules()
{
    HighlightingRule rule;

    // Keywords
    const QString keywordPatterns[] = {
        "\\bAND\\b", "\\bARRAY\\b", "\\bASM\\b", "\\bBEGIN\\b", "\\bCASE\\b", "\\bCONST\\b",
        "\\bDIV\\b", "\\bDO\\b", "\\bDOWNTO\\b", "\\bELSE\\b", "\\bEND\\b", "\\bFILE\\b",
        "\\bFOR\\b", "\\bFUNCTION\\b", "\\bGOTO\\b", "\\bIF\\b", "\\bIN\\b", "\\bLABEL\\b",
        "\\bMOD\\b", "\\bNIL\\b", "\\bNOT\\b", "\\bOF\\b", "\\bOR\\b", "\\bPACKED\\b",
        "\\bPROCEDURE\\b", "\\bPROGRAM\\b", "\\bRECORD\\b", "\\bREPEAT\\b", "\\bSET\\b",
        "\\bTHEN\\b", "\\bTO\\b", "\\bTYPE\\b", "\\bUNIT\\b", "\\bUNTIL\\b", "\\bVAR\\b",
        "\\bWHILE\\b", "\\bWITH\\b"
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Types
    const QString typePatterns[] = {
        "\\bBOOLEAN\\b", "\\bBYTE\\b", "\\bCHAR\\b", "\\bEXTENDED\\b", "\\bINTEGER\\b",
        "\\bLONGINT\\b", "\\bREAL\\b", "\\bSHORTINT\\b", "\\bSINGLE\\b", "\\bSMALLINT\\b",
        "\\bSTRING\\b", "\\bTEXT\\b", "\\bWORD\\b", "\\bPOINTER\\b"
    };
    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = typeFormat;
        highlightingRules.append(rule);
    }

    // Operators (e.g., :=, +, -, *, /, <, >, =, <=, >=, <>)
    rule.pattern = QRegularExpression("(:=|\\+|\\-|\\*|/|=|<>|<|<=|>|>=)");
    rule.format = operatorFormat;
    highlightingRules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("(\"(\\\\.|[^\"])*\"|'(\\\\.|[^'])*')");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    // Numbers (integers and floating-point)
    rule.pattern = QRegularExpression("\\b[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // Single-line Comments (// style for Free Pascal, or just for consistency)
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    // Preprocessor Directives (Free Pascal style)
    rule.pattern = QRegularExpression("\\{\\$(?:[A-Za-z_][A-Za-z0-9_]*)\\b");
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);
}

void PascalSyntaxHighlighter::highlightBlock(const QString &text)
{
    // Fix the warning: Use std::as_const instead of qAsConst
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0); // Reset block state for multi-line comments

    int startIndex = 0;
    if (previousBlockState() != 1) { // If not inside a multi-line comment from previous block
        startIndex = text.indexOf(commentStartExpression);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) { // End comment not found in this block
            setCurrentBlockState(1); // Mark as inside multi-line comment
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(startIndex, commentLength, commentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}