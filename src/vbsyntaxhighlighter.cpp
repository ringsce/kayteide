#include "vbsyntaxhighlighter.h"
#include <QTextDocument>
#include <QRegularExpression>
#include <QColor>
#include <QString> // Make sure QString is included for .contains()

VBSyntaxHighlighter::VBSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
    setupKeywords();
}

void VBSyntaxHighlighter::setupFormats()
{
    keywordFormat.setForeground(QColor(Qt::blue));
    keywordFormat.setFontWeight(QFont::Bold);

    typeFormat.setForeground(QColor(0, 192, 192)); // A brighter cyan
    typeFormat.setFontWeight(QFont::Normal);

    directiveFormat.setForeground(QColor(Qt::magenta));
    directiveFormat.setFontWeight(QFont::Bold);

    functionFormat.setForeground(QColor(Qt::cyan));
    functionFormat.setFontWeight(QFont::Normal);

    operatorFormat.setForeground(QColor(Qt::red));
    operatorFormat.setFontWeight(QFont::Normal);

    commentFormat.setForeground(QColor(Qt::darkGreen));
    commentFormat.setFontWeight(QFont::Normal);

    stringFormat.setForeground(QColor(Qt::darkYellow));
    stringFormat.setFontWeight(QFont::Normal);

    numberFormat.setForeground(QColor(Qt::magenta));
    numberFormat.setFontWeight(QFont::Normal);

    identifierFormat.setForeground(QColor(Qt::black));
    identifierFormat.setFontWeight(QFont::Normal);

    preprocessorFormat.setForeground(QColor(Qt::darkGray));
    preprocessorFormat.setFontWeight(QFont::Normal);
}

void VBSyntaxHighlighter::setupKeywords()
{
    keywords << "if" << "then" << "else" << "elseif" << "end if"
             << "for" << "next" << "while" << "wend"
             << "do" << "loop" << "until" << "exit do" << "exit for" << "exit function" << "exit property" << "exit sub"
             << "select" << "case" << "end select"
             << "sub" << "end sub" << "function" << "end function"
             << "property" << "end property" << "class" << "end class"
             << "module" << "end module" << "structure" << "end structure"
             << "enum" << "end enum"
             << "dim" << "as" << "new" << "set" << "nothing"
             << "public" << "private" << "protected" << "friend"
             << "static" << "const" << "withevents"
             << "true" << "false" << "not" << "and" << "or" << "xor"
             << "byval" << "byref" << "optional" << "paramarray"
             << "call" << "goto" << "resume" << "stop" << "end"
             << "on error" << "gosub" << "return"
             << "with" << "end with" << "synclock" << "end synclock"
             << "is" << "like" << "mod"
             << "me" << "mybase" << "myclass" << "shared" << "overrides" << "overloads"
             << "handles" << "addhandler" << "removehandler" << "raiseevent"
             << "implements" << "inherits"
             << "try" << "catch" << "finally" << "end try"
             << "throw"
             << "yield" << "await" << "async"
             << "using" << "end using"
             << "get" << "set"
             << "addressof"
             << "typeof"
             << "print" << "input" << "rem";

    types << "integer" << "long" << "single" << "double"
          << "string" << "boolean" << "date" << "object"
          << "variant" << "byte" << "currency" << "decimal"
          << "char" << "short" << "ulong" << "ushort" << "uinteger"
          << "sbyte" << "dbnull" << "void" << "any";

    directives << "#if" << "#else" << "#elseif" << "#end if"
               << "#const" << "#externalsource" << "#region" << "#end region"
               << "#disable warning" << "#enable warning";

    functions << "msgbox" << "inputbox" << "len" << "mid"
              << "left" << "right" << "instr" << "format"
              << "cdate" << "now" << "datediff" << "isempty"
              << "isnumeric" << "isdate" << "isobject" << "isnull"
              << "trim" << "ltrim" << "rtrim" << "replace"
              << "ubound" << "lbound" << "array"
              << "chr" << "asc" << "string" << "space"
              << "rnd" << "randomize" << "sqr" << "abs"
              << "sin" << "cos" << "tan" << "atn" << "log" << "exp"
              << "fix" << "int" << "round"
              << "createobject" << "getobject"
              << "val" << "str" << "hex" << "oct"
              << "strtointdef" << "inttostr" << "splitstring";

    operators << "<>" << ">=" << "<=";
}

void VBSyntaxHighlighter::highlightBlock(const QString &text)
{
    int length = text.length();

    // 1. Handle "Rem" and "'" comments
    // Single quote comment
    QRegularExpression commentRx("'", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch commentMatch = commentRx.match(text);
    if (commentMatch.hasMatch()) {
        int commentStart = commentMatch.capturedStart();
        setFormat(commentStart, length - commentStart, commentFormat);
        return; // Rest of the line is a comment
    }

    // "Rem" comment (needs to be a whole word, potentially at start or after space)
    QRegularExpression remCommentRx("\\brem\\b", QRegularExpression::CaseInsensitiveOption);
    commentMatch = remCommentRx.match(text, 0); // No PartialMatch option, just match from start
    if (commentMatch.hasMatch()) {
        int remStart = commentMatch.capturedStart();
        // Check if "rem" is at the start or preceded by whitespace, to avoid false positives like "remember"
        if (remStart == 0 || (remStart > 0 && text.at(remStart - 1).isSpace())) {
            setFormat(remStart, length - remStart, commentFormat);
            return;
        }
    }

    // 2. Handle strings (quoted literals)
    // This regex needs to handle escaped quotes ("" in VB)
    // This regex means: " (start quote) then any character except " or \, OR a backslash followed by any character (for escapes), zero or more times, then " (end quote)
    // For VB, double quotes are escaped by doubling them, not backslash.
    // Corrected regex for VB strings: "((?:[^"]|"")*)"
    QRegularExpression stringRx("\"((?:[^\\\"]|\\\"\\\")*)\"", QRegularExpression::DotMatchesEverythingOption); // DotMatchesEverything to allow newlines (though highlightBlock is line-by-line)
    QRegularExpressionMatchIterator stringIt = stringRx.globalMatch(text);
    while (stringIt.hasNext()) {
        QRegularExpressionMatch match = stringIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), stringFormat);
    }

    // 3. Handle numbers (integers, floats, hex, octal)
    QRegularExpression numberRx("\\b(?:(?:&H[0-9a-fA-F]+)|(?:&O[0-7]+)|(?:\\d*\\.?\\d+(?:[eE][+-]?\\d+)?))[!#$%&ILR@]?\\b", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator numberIt = numberRx.globalMatch(text);
    while (numberIt.hasNext()) {
        QRegularExpressionMatch match = numberIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), numberFormat);
    }

    // 4. Handle Preprocessor directives (e.g., #If, #Const)
    QRegularExpression preprocessorRx("^\\s*#\\w+", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch preprocessorMatch = preprocessorRx.match(text);
    if (preprocessorMatch.hasMatch()) {
        setFormat(preprocessorMatch.capturedStart(), preprocessorMatch.capturedLength(), preprocessorFormat);
    }

    // 5. Handle Keywords, Types, Functions, Identifiers, Operators
    int startIndex = 0;
    while (startIndex < length) {
        QChar c = text.at(startIndex);

        // Skip characters already covered by previous regex matches (strings, numbers, comments, preprocessors)
        // This is a basic attempt to not re-highlight.
        // A more robust solution might involve iterating through textCharFormat changes,
        // or using a more sophisticated tokenization approach.
        // For now, let's just skip if it's already got one of the complex formats.
        // Note: format() method is part of QSyntaxHighlighter
        QTextCharFormat existingFormat = format(startIndex);
        if (existingFormat.foreground() == commentFormat.foreground() ||
            existingFormat.foreground() == stringFormat.foreground() ||
            existingFormat.foreground() == numberFormat.foreground() ||
            existingFormat.foreground() == preprocessorFormat.foreground())
        {
            startIndex++;
            continue;
        }


        // Handle multi-character operators first
        bool matchedOperator = false;
        for (const QString& op : operators) {
            if (text.mid(startIndex, op.length()).compare(op, Qt::CaseInsensitive) == 0) {
                setFormat(startIndex, op.length(), operatorFormat);
                startIndex += op.length();
                matchedOperator = true;
                break;
            }
        }
        if (matchedOperator) continue;

        // Handle single-character operators/symbols
        if (QString("=+-*/\\^&()[],;:.!").contains(c)) { // Corrected: use QString literal
            setFormat(startIndex, 1, operatorFormat);
            startIndex++;
            continue;
        }

        // Handle Identifiers, Keywords, Types, Functions
        if (c.isLetter() || c == '_') {
            int i = startIndex;
            while (i < length && (text.at(i).isLetterOrNumber() || text.at(i) == '_')) {
                i++;
            }
            int tokenLength = i - startIndex;
            QString currentToken = text.mid(startIndex, tokenLength);
            QString lowerToken = currentToken.toLower();

            if (keywords.contains(lowerToken)) {
                setFormat(startIndex, tokenLength, keywordFormat);
            } else if (types.contains(lowerToken)) {
                setFormat(startIndex, tokenLength, typeFormat);
            } else if (functions.contains(lowerToken)) {
                setFormat(startIndex, tokenLength, functionFormat);
            } else if (directives.contains("#" + lowerToken)) {
                 setFormat(startIndex, tokenLength, directiveFormat);
            } else {
                setFormat(startIndex, tokenLength, identifierFormat);
            }
            startIndex += tokenLength;
            continue;
        }

        // Move past unhandled characters (e.g., whitespace, or unknown chars)
        startIndex++;
    }
}