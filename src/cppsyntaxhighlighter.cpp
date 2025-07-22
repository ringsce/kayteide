#include "cppsyntaxhighlighter.h"
#include <QTextDocument>
#include <QRegularExpression>
#include <QColor>
#include <QString>
#include <utility> // <--- ADD THIS LINE FOR std::as_const

CppSyntaxHighlighter::CppSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
    setupRules();
}

void CppSyntaxHighlighter::setupFormats()
{
    keywordFormat.setForeground(QColor("#0000FF")); // Blue
    keywordFormat.setFontWeight(QFont::Bold);

    typeFormat.setForeground(QColor("#008080")); // Teal
    typeFormat.setFontWeight(QFont::Normal);

    classFormat.setForeground(QColor("#2B91AF")); // Light Blue/Cyan
    classFormat.setFontWeight(QFont::Bold);

    singleLineCommentFormat.setForeground(QColor("#008000")); // Green
    singleLineCommentFormat.setFontItalic(true);

    multiLineCommentFormat.setForeground(QColor("#008000")); // Green
    multiLineCommentFormat.setFontItalic(true);

    quotationFormat.setForeground(QColor("#A31515")); // Dark Red
    quotationFormat.setFontWeight(QFont::Normal);

    functionFormat.setForeground(QColor("#795E26")); // Brownish
    functionFormat.setFontWeight(QFont::Normal);

    preprocessorFormat.setForeground(QColor("#800000")); // Maroon
    preprocessorFormat.setFontWeight(QFont::Bold);

    numberFormat.setForeground(QColor("#B000B0")); // Purple
    numberFormat.setFontWeight(QFont::Normal);

    operatorFormat.setForeground(QColor("#FF0000")); // Red
    operatorFormat.setFontWeight(QFont::Normal);

    booleanFormat.setForeground(QColor("#FF8C00")); // Dark Orange
    booleanFormat.setFontWeight(QFont::Bold);

    nullptrFormat.setForeground(QColor("#FF8C00")); // Dark Orange (same as boolean for consistency)
    nullptrFormat.setFontWeight(QFont::Bold);
}

void CppSyntaxHighlighter::setupRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns;
    keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b" << "\\bconstexpr\\b"
                    << "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b" << "\\bextern\\b"
                    << "\\bfloat\\b" << "\\binline\\b" << "\\bint\\b" << "\\blong\\b"
                    << "\\bnamespace\\b" << "\\boperator\\b" << "\\bprivate\\b" << "\\bprotected\\b"
                    << "\\bpublic\\b" << "\\bshort\\b" << "\\bsigned\\b" << "\\bsizeof\\b"
                    << "\\bstatic\\b" << "\\bstruct\\b" << "\\btemplate\\b" << "\\bthis\\b"
                    << "\\btypedef\\b" << "\\btypename\\b" << "\\bunion\\b" << "\\bunsigned\\b"
                    << "\\bvirtual\\b" << "\\bvoid\\b" << "\\bvolatile\\b" << "\\bbool\\b"
                    << "\\bauto\\b" << "\\bdecltype\\b" << "\\bthread_local\\b" << "\\bnoexcept\\b"
                    << "\\bstatic_assert\\b" << "\\bconcept\\b" << "\\bco_await\\b" << "\\bco_yield\\b"
                    << "\\bco_return\\b" << "\\brequires\\b" << "\\bconcept\\b" << "\\bexport\\b"
                    << "\\bmodule\\b" << "\\bimport\\b" << "\\bconstinit\\b" << "\\bconsteval\\b"
                    << "\\bif\\b" << "\\belse\\b" << "\\bswitch\\b" << "\\bcase\\b" << "\\bdefault\\b"
                    << "\\bfor\\b" << "\\bwhile\\b" << "\\bdo\\b" << "\\bbreak\\b" << "\\bcontinue\\b"
                    << "\\bgoto\\b" << "\\breturn\\b" << "\\btry\\b" << "\\bcatch\\b" << "\\bthrow\\b"
                    << "\\busing\\b" << "\\bnew\\b" << "\\bdelete\\b" << "\\bfriend\\b"
                    << "\\bconcept\\b" << "\\btypename\\b" << "\\boverride\\b" << "\\bfinal\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // C++ Types (primitive + common std types)
    QStringList typePatterns;
    typePatterns << "\\bbool\\b" << "\\bchar\\b" << "\\bint\\b" << "\\bfloat\\b"
                 << "\\bdouble\\b" << "\\bvoid\\b" << "\\bshort\\b" << "\\blong\\b"
                 << "\\bsigned\\b" << "\\bunsigned\\b" << "\\bstd::string\\b"
                 << "\\bstd::vector\\b" << "\\bstd::map\\b" << "\\bstd::set\\b"
                 << "\\bstd::unique_ptr\\b" << "\\bstd::shared_ptr\\b"
                 << "\\bstd::function\\b" << "\\bauto\\b"; // auto is also a type deduction keyword
    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = typeFormat;
        highlightingRules.append(rule);
    }

    // Classes (Qt classes and common C++ standard library classes)
    QStringList classPatterns;
    classPatterns << "\\bQ[A-Za-z]+\\b" // Matches Qt classes like QApplication, QWidget
                  << "\\bstd::[A-Za-z]+\\b" // Matches standard library classes like std::vector
                  << "\\bstring\\b" << "\\bvector\\b" << "\\bmap\\b" << "\\bset\\b" // Common standalone uses
                  << "\\bunique_ptr\\b" << "\\bshared_ptr\\b";
    for (const QString &pattern : classPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = classFormat;
        highlightingRules.append(rule);
    }

    // Operators
    QStringList operatorPatterns;
    operatorPatterns << "\\b(and|or|not|xor|bitand|bitor|compl|not_eq|or_eq|xor_eq)\\b" // Textual operators
                     << "=" << "==" << "!=" << "<" << "<=" << ">" << ">="
                     << "\\+" << "-" << "\\*" << "/" << "%" // Arithmetic
                     << "&" << "\\|" << "\\^" << "<<" << ">>" // Bitwise and shifts
                     << "&&" << "\\|\\|" << "!" // Logical
                     << "\\+=" << "-=" << "\\*=" << "/=" << "%="
                     << "&=" << "\\|=" << "\\^=" << "<<=" << ">>=" // Assignment operators
                     << "\\+\\+" << "--" // Increment/Decrement
                     << "->\\*" << "->" << "\\." << "\\.\\*" // Pointers, member access
                     << "::" // Scope resolution
                     << "\\?" << ":" // Ternary
                     << "\\bnew\\b" << "\\bdelete\\b" << "\\btypeid\\b" << "\\bstatic_cast\\b"
                     << "\\breinterpret_cast\\b" << "\\bconst_cast\\b" << "\\bdynamic_cast\\b";
    for (const QString &pattern : operatorPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = operatorFormat;
        highlightingRules.append(rule);
    }

    // Booleans
    rule.pattern = QRegularExpression("\\b(true|false)\\b");
    rule.format = booleanFormat;
    highlightingRules.append(rule);

    // nullptr
    rule.pattern = QRegularExpression("\\bnullptr\\b");
    rule.format = nullptrFormat;
    highlightingRules.append(rule);

    // Numbers (integer, float, hex, octal, binary)
    rule.pattern = QRegularExpression("\\b(?:0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]*|[1-9][0-9]*|\\d*\\.\\d+(?:[eE][+-]?\\d+)?)(?:[uU][lL]{0,2}|[lL]{0,2}[uU])?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // Double-quoted strings
    rule.pattern = QRegularExpression("\"(?:\\\\.|[^\\\\\"])*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Single-quoted character literals
    rule.pattern = QRegularExpression("'[^\']'"); // Simple char: 'a'
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Functions (identifies something followed by '(')
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Preprocessor directives (#include, #define, etc.)
    rule.pattern = QRegularExpression("^\\s*#\\s*(?:include|define|undef|if|ifdef|ifndef|else|elif|endif|error|pragma|line)\\b.*");
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);

    // Multi-line comments
    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void CppSyntaxHighlighter::highlightBlock(const QString &text)
{
    // Apply highlighting rules
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) { // <--- Changed qAsConst to std::as_const
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Multi-line comments
    setCurrentBlockState(0); // Reset block state for current line

    int startIndex = 0;
    if (previousBlockState() != 1) { // If previous block was not inside a multi-line comment
        startIndex = text.indexOf(commentStartExpression);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) { // No end delimiter found on this line, comment continues
            setCurrentBlockState(1); // Set state to indicate inside multi-line comment
            commentLength = text.length() - startIndex;
        } else { // End delimiter found
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}