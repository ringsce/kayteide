#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>
#include <QFont>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    enum Language { Generic, C_CPP, PHP, Perl, Pascal, Kayte };

    explicit SyntaxHighlighter(QTextDocument *doc, Language lang = Generic);
    void setLanguage(Language lang);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };
    QVector<Rule> m_rules;

    QTextCharFormat m_commentFmt;
    QTextCharFormat m_stringFmt;
    QTextCharFormat m_keywordFmt;
    QTextCharFormat m_numberFmt;
    QTextCharFormat m_funcFmt;
    QTextCharFormat m_preprocessFmt;

    void buildRules(Language lang);
    void addRule(const QString &pattern, const QTextCharFormat &fmt,
                 QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption);
};
