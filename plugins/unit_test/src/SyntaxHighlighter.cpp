#include "SyntaxHighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *doc, Language lang)
    : QSyntaxHighlighter(doc)
{
    // Set up formats
    m_keywordFmt.setForeground(QColor("#cba6f7")); // mauve
    m_keywordFmt.setFontWeight(QFont::Bold);

    m_stringFmt.setForeground(QColor("#a6e3a1"));  // green

    m_commentFmt.setForeground(QColor("#6c7086")); // overlay0
    m_commentFmt.setFontItalic(true);

    m_numberFmt.setForeground(QColor("#fab387")); // peach

    m_funcFmt.setForeground(QColor("#89b4fa"));   // blue

    m_preprocessFmt.setForeground(QColor("#f38ba8")); // red

    buildRules(lang);
}

void SyntaxHighlighter::setLanguage(Language lang)
{
    m_rules.clear();
    buildRules(lang);
    rehighlight();
}


void SyntaxHighlighter::buildRules(Language lang)
{
    switch (lang) {

    case C_CPP: {
        QStringList kw = {"auto","break","case","char","const","continue","default",
                          "do","double","else","enum","extern","float","for","goto",
                          "if","inline","int","long","register","return","short",
                          "signed","sizeof","static","struct","switch","typedef",
                          "union","unsigned","void","volatile","while",
                          // C++
                          "class","namespace","template","typename","public","private",
                          "protected","virtual","override","final","nullptr","true",
                          "false","new","delete","try","catch","throw","using",
                          "explicit","operator","friend","constexpr","auto","decltype"};
        addRule("\\b(" + kw.join('|') + ")\\b", m_keywordFmt);
        addRule("#\\s*\\w+", m_preprocessFmt);
        addRule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", m_stringFmt);
        addRule("'[^'\\\\]*(\\\\.[^'\\\\]*)*'", m_stringFmt);
        addRule("//[^\n]*", m_commentFmt);
        addRule("/\\*.*\\*/", m_commentFmt);
        addRule("\\b[0-9]+(\\.[0-9]+)?\\b", m_numberFmt);
        addRule("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()", m_funcFmt);
        break;
    }

    case PHP: {
        QStringList kw = {"echo","print","if","else","elseif","while","for","foreach",
                          "do","switch","case","break","continue","return","function",
                          "class","extends","implements","new","null","true","false",
                          "public","private","protected","static","abstract","final",
                          "try","catch","throw","use","namespace","interface","trait"};
        addRule("\\b(" + kw.join('|') + ")\\b", m_keywordFmt);
        addRule("\\$[A-Za-z_][A-Za-z0-9_]*", m_funcFmt);
        addRule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", m_stringFmt);
        addRule("'[^'\\\\]*(\\\\.[^'\\\\]*)*'", m_stringFmt);
        addRule("//[^\n]*|#[^\n]*", m_commentFmt);
        addRule("\\b[0-9]+(\\.[0-9]+)?\\b", m_numberFmt);
        break;
    }

    case Perl: {
        QStringList kw = {"if","elsif","else","unless","while","until","for","foreach",
                          "do","last","next","redo","return","sub","my","local","our",
                          "use","require","package","print","say","die","warn",
                          "push","pop","shift","unshift","splice","reverse","sort",
                          "map","grep","chomp","chop","length","defined","undef",
                          "ref","scalar","wantarray"};
        addRule("\\b(" + kw.join('|') + ")\\b", m_keywordFmt);
        addRule("[\\$@%][A-Za-z_][A-Za-z0-9_]*", m_funcFmt);
        addRule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", m_stringFmt);
        addRule("'[^'\\\\]*'", m_stringFmt);
        addRule("#[^\n]*", m_commentFmt);
        addRule("\\b[0-9]+(\\.[0-9]+)?\\b", m_numberFmt);
        break;
    }

    case Pascal: {
        QStringList kw = {"program","uses","var","const","type","begin","end",
                          "procedure","function","if","then","else","while","do",
                          "for","to","downto","repeat","until","case","of","with",
                          "record","array","string","integer","real","boolean",
                          "char","true","false","nil","not","and","or","xor",
                          "div","mod","writeln","write","readln","read","exit"};
        addRule("\\b(" + kw.join('|') + ")\\b", m_keywordFmt,
                QRegularExpression::CaseInsensitiveOption);
        addRule("'[^']*'", m_stringFmt);
        addRule("\\{[^}]*\\}", m_commentFmt);
        addRule("\\(\\*.*\\*\\)", m_commentFmt);
        addRule("//[^\n]*", m_commentFmt);
        addRule("\\b[0-9]+(\\.[0-9]+)?\\b", m_numberFmt);
        break;
    }

    case Kayte: {
        // BASIC-style: keywords are case-insensitive
        QStringList kw = {"DIM","AS","PRINT","INPUT","IF","THEN","ELSE","ELSEIF",
                          "END","FOR","TO","STEP","NEXT","WHILE","WEND","DO",
                          "LOOP","UNTIL","GOTO","GOSUB","RETURN","SUB","FUNCTION",
                          "LET","REM","OPTION","EXPLICIT","ON","OFF","STRING",
                          "INTEGER","BOOLEAN","SINGLE","DOUBLE","AND","OR","NOT",
                          "MOD","TRUE","FALSE","SELECT","CASE","EXIT"};
        addRule("\\b(" + kw.join('|') + ")\\b", m_keywordFmt,
                QRegularExpression::CaseInsensitiveOption);
        addRule("\"[^\"]*\"", m_stringFmt);
        addRule("'[^\n]*", m_commentFmt);   // inline comment
        addRule("\\b[0-9]+(\\.[0-9]+)?\\b", m_numberFmt);
        addRule("\\b[A-Za-z_][A-Za-z0-9_]*\\b", m_funcFmt); // identifiers
        break;
    }

    default:
        addRule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", m_stringFmt);
        addRule("#[^\n]*|//[^\n]*", m_commentFmt);
        addRule("\\b[0-9]+\\b", m_numberFmt);
        break;
    }
}

void SyntaxHighlighter::addRule(const QString &pattern, const QTextCharFormat &fmt,
                                QRegularExpression::PatternOptions opts)
{
    Rule r;
    r.pattern = QRegularExpression(pattern, opts);
    r.format  = fmt;
    m_rules.append(r);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const Rule &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.format);
        }
    }
}
