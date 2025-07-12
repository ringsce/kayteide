#include "delphisyntaxhighlighter.h"

DelphiSyntaxHighlighter::DelphiSyntaxHighlighter(QTextDocument *parent)
    : PascalSyntaxHighlighter(parent) // Call base class constructor
{
    setupDelphiFormats();
    setupDelphiRules();

    // Re-initialize multi-line comment expressions if needed, or rely on base
    // commentStartExpression = QRegularExpression("\\{\\s*");
    // commentEndExpression = QRegularExpression("\\s*\\}");
}

void DelphiSyntaxHighlighter::setupDelphiFormats()
{
    // Delphi-specific Keywords
    delphiKeywordFormat.setForeground(QColor(0, 0, 150)); // Slightly different dark blue
    delphiKeywordFormat.setFontWeight(QFont::ExtraBold); // More prominent

    // Class/Interface names (often a specific color in IDEs)
    classFormat.setForeground(QColor(128, 0, 64)); // Dark Magenta
    classFormat.setFontWeight(QFont::Bold);

    // Interface names (can be same or different from class)
    interfaceFormat.setForeground(QColor(64, 0, 128)); // Dark Purple
    interfaceFormat.setFontWeight(QFont::Bold);

    // Attributes/Annotations (Delphi specific, if you want to highlight {<attr>})
    attributeFormat.setForeground(QColor(0, 120, 120)); // Teal
    attributeFormat.setFontItalic(true);
}

void DelphiSyntaxHighlighter::setupDelphiRules()
{
    HighlightingRule rule;

    // --- Delphi-specific Keywords (extend Pascal's keywords) ---
    // These are often case-insensitive as well in Delphi
    const QString delphiKeywordPatterns[] = {
        "\\bCLASS\\b", "\\bINTERFACE\\b", "\\bIMPLEMENTATION\\b", "\\bINHERITED\\b",
        "\\bOVERRIDE\\b", "\\bVIRTUAL\\b", "\\bABSTRACT\\b", "\\bPROPERTY\\b",
        "\\bREAD\\b", "\\bWRITE\\b", "\\bINDEX\\b", "\\bDEFAULT\\b", "\\bIMPLEMENTS\\b",
        "\\bPUBLISHED\\b", "\\bPROTECTED\\b", "\\bPRIVATE\\b", "\\bPUBLIC\\b",
        "\\bSTRICTPRIVATE\\b", "\\bSTRICTPROTECTED\\b", "\\bDISPINTERFACE\\b",
        "\\bMESSAGE\\b", "\\bAS\\b", "\\bIS\\b", "\\bTRY\\b", "\\bFINALLY\\b",
        "\\bEXCEPT\\b", "\\bRAISE\\b", "\\bWITH\\b", "\\bON\\b", "\\bRESIDENT\\b",
        "\\bTHREADVAR\\b", "\\bFINAL\\b", "\\bSEALED\\b", "\\bOVERLOAD\\b",
        "\\bHELPER\\b", "\\bEXTENSION\\b", "\\bASSERT\\b"
    };

    for (const QString &pattern : delphiKeywordPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = delphiKeywordFormat; // Use the specific Delphi format
        highlightingRules.append(rule); // Add to the base class's rules
    }

    // --- Common Delphi Class/Interface/Enum declarations (example of structural highlighting) ---
    // Highlighting 'TMyClass', 'IMyInterface', 'TMyEnum' after 'CLASS', 'INTERFACE', 'TYPE' keywords
    // This is more complex and often involves lookbehind/lookahead or a more sophisticated lexer.
    // For simplicity, here are patterns for common naming conventions if desired.
    rule.format = classFormat;
    rule.pattern = QRegularExpression("\\bT[A-Z][a-zA-Z0-9_]*\\b"); // Matches TMyClass, TForm1 etc.
    highlightingRules.append(rule);

    rule.format = interfaceFormat;
    rule.pattern = QRegularExpression("\\bI[A-Z][a-zA-Z0-9_]*\\b"); // Matches IMyInterface etc.
    highlightingRules.append(rule);

    // Attributes (e.g., {<AttributeName>})
    rule.pattern = QRegularExpression("\\{\\<[A-Za-z_][A-Za-z0-9_]*\\>\\}");
    rule.format = attributeFormat;
    highlightingRules.append(rule);

    // Multi-line comments: (* ... *)
    // Delphi often uses { } comments for single-line, but also supports (* *) for multi-line.
    // The base class handles {}, you might want to add (* *) here.
    rule.pattern = QRegularExpression("\\(\\*.*?\\*\\)"); // Non-greedy match for multi-line
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void DelphiSyntaxHighlighter::highlightBlock(const QString &text)
{
    // First, let the Pascal highlighter do its job for common rules
    PascalSyntaxHighlighter::highlightBlock(text);

    // Then, apply Delphi-specific rules on top
    // Iterate through the rules specifically added by setupDelphiRules
    // Note: This approach adds Delphi rules to the base `highlightingRules` vector.
    // If you want strict separation and control over precedence, you might
    // maintain a separate `delphiHighlightingRules` vector and iterate it here.
    // For simplicity with inheritance, we're assuming the base `highlightingRules`
    // vector gets appended to.

    // Re-apply common rules with potential Delphi-specific overrides if needed.
    // This part is crucial for making sure Delphi-specific highlighting
    // (like bolder keywords) overrides any general Pascal rules.
    // The order of rules in `highlightingRules` matters. Rules added later
    // will override earlier ones if they match the same text.
    // If a Delphi keyword (e.g., 'CLASS') was first matched by a generic Pascal keyword rule,
    // and then later by a more specific Delphi keyword rule, the latter format will apply.
    // Ensure `setupDelphiRules` appends its rules after `setupRules` implicitly via constructor chain.
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        // This loop applies ALL rules, including those inherited/set by PascalSyntaxHighlighter
        // This implicitly handles overrides if rules are ordered such that
        // more specific Delphi rules come after general Pascal rules.
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Re-run multi-line comment logic if Delphi adds new types of comments or needs different logic.
    // For (* *) comments, if not handled by a simple regex in setupDelphiRules,
    // you'd need similar multi-line logic to the base class.
    // However, QRegularExpression("\\(\\*.*?\\*\\)") typically covers them within highlightBlock's loop.
    // If you want nested or specific multi-line handling for (* *), you'd repeat the multi-line logic here.
}