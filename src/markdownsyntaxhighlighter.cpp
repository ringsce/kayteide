#include "markdownsyntaxhighlighter.h"

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>

namespace {
bool darkTheme()
{
    return QGuiApplication::palette().color(QPalette::Base).lightness() < 128;
}

QTextCharFormat colored(const QColor &c, bool bold = false, bool italic = false)
{
    QTextCharFormat f;
    f.setForeground(c);
    if (bold) f.setFontWeight(QFont::Bold);
    if (italic) f.setFontItalic(true);
    return f;
}
} // namespace

MarkdownSyntaxHighlighter::MarkdownSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    const bool dark = darkTheme();
    const QColor heading = dark ? QColor(0x6c, 0xb6, 0xff) : QColor(0x05, 0x50, 0xae);
    const QColor code    = dark ? QColor(0xe6, 0xa5, 0x6b) : QColor(0xa3, 0x40, 0x0c);
    const QColor link    = dark ? QColor(0x7e, 0xc6, 0x99) : QColor(0x1a, 0x7f, 0x37);
    const QColor dim     = dark ? QColor(0x8b, 0x94, 0x9e) : QColor(0x6e, 0x77, 0x81);
    const QColor marker  = dark ? QColor(0xd2, 0x8f, 0xe8) : QColor(0x82, 0x50, 0xdf);

    m_heading     = colored(heading, true);
    m_code        = colored(code);
    m_code.setFontFamilies({QFontDatabase::systemFont(QFontDatabase::FixedFont).family()});
    m_fenceMarker = colored(dim);
    m_quote       = colored(dim, false, true);
    m_listMarker  = colored(marker, true);
    m_rule        = colored(dim, true);

    const auto add = [this](const char *pattern, const QTextCharFormat &format, int group = 0) {
        m_inlineRules.append({QRegularExpression(QString::fromLatin1(pattern)), format, group});
    };

    // Emphasis first, so code spans and links (added later) win where they overlap.
    add(R"((\*\*|__)(?=\S)(.+?)(?<=\S)\1)", colored(QGuiApplication::palette().color(QPalette::Text), true));
    add(R"((?<![*\w])\*(?=[^\s*])(.+?)(?<=[^\s*])\*(?![*\w]))",
        colored(QGuiApplication::palette().color(QPalette::Text), false, true));
    add(R"((?<![_\w])_(?=[^\s_])(.+?)(?<=[^\s_])_(?![_\w]))",
        colored(QGuiApplication::palette().color(QPalette::Text), false, true));
    add(R"(~~(?=\S)(.+?)(?<=\S)~~)", [&] {
        QTextCharFormat f = colored(dim);
        f.setFontStrikeOut(true);
        return f;
    }());
    add(R"(</?[A-Za-z][^>\n]*>)", colored(dim));                         // inline HTML
    add(R"(<(?:https?|mailto|ftp):[^>\s]+>)", colored(link));             // autolinks
    add(R"(!?\[[^\]\n]*\])", colored(link));                              // [text] / ![alt]
    add(R"((?<=\])\([^)\n]*\))", colored(dim));                           // (url "title")
    add(R"(^\s*\[[^\]\n]+\]:\s*\S+.*$)", colored(dim));                   // [ref]: url
    add(R"(`[^`\n]+`)", m_code);                                          // inline code
}

void MarkdownSyntaxHighlighter::highlightBlock(const QString &text)
{
    static const QRegularExpression fence(QStringLiteral(R"(^\s{0,3}(```|~~~))"));

    // ── Fenced code blocks span lines: track them in the block state ─────────
    const bool fenceLine = fence.match(text).hasMatch();
    if (previousBlockState() == InFence) {
        setFormat(0, int(text.length()), fenceLine ? m_fenceMarker : m_code);
        setCurrentBlockState(fenceLine ? Normal : InFence);
        return;
    }
    if (fenceLine) {
        setFormat(0, int(text.length()), m_fenceMarker);
        setCurrentBlockState(InFence);
        return;
    }
    setCurrentBlockState(Normal);

    // ── Whole-line constructs ────────────────────────────────────────────────
    static const QRegularExpression heading(QStringLiteral(R"(^\s{0,3}#{1,6}(\s|$))"));
    static const QRegularExpression setext(QStringLiteral(R"(^\s{0,3}(=+|-+)\s*$)"));
    static const QRegularExpression hrule(QStringLiteral(R"(^\s{0,3}([-*_])(\s*\1){2,}\s*$)"));
    static const QRegularExpression quote(QStringLiteral(R"(^\s{0,3}>)"));
    static const QRegularExpression indentedCode(QStringLiteral(R"(^(    |\t)\S)"));

    if (heading.match(text).hasMatch()) {
        setFormat(0, int(text.length()), m_heading);
        return;
    }
    if (hrule.match(text).hasMatch()) {
        setFormat(0, int(text.length()), m_rule);
        return;
    }
    // "Title\n=====" — underline makes the previous line a heading; colour the
    // underline itself (the previous block was already highlighted).
    if (setext.match(text).hasMatch() && currentBlock().previous().isValid()
        && !currentBlock().previous().text().trimmed().isEmpty()) {
        setFormat(0, int(text.length()), m_heading);
        return;
    }
    if (indentedCode.match(text).hasMatch()
        && (!currentBlock().previous().isValid()
            || currentBlock().previous().text().trimmed().isEmpty()
            || indentedCode.match(currentBlock().previous().text()).hasMatch())) {
        setFormat(0, int(text.length()), m_code);
        return;
    }
    if (quote.match(text).hasMatch())
        setFormat(0, int(text.length()), m_quote);

    // ── List markers and task boxes ──────────────────────────────────────────
    static const QRegularExpression list(QStringLiteral(R"(^\s*([-*+]|\d{1,9}[.)])\s+(\[[ xX]\]\s)?)"));
    if (const auto m = list.match(text); m.hasMatch())
        setFormat(int(m.capturedStart()), int(m.capturedLength()), m_listMarker);

    // Table separator rows (| --- | :---: |).
    static const QRegularExpression tableRule(QStringLiteral(R"(^\s*\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)+\|?\s*$)"));
    if (tableRule.match(text).hasMatch()) {
        setFormat(0, int(text.length()), m_rule);
        return;
    }

    // ── Inline spans ─────────────────────────────────────────────────────────
    for (const Rule &rule : m_inlineRules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            setFormat(int(m.capturedStart(rule.group)), int(m.capturedLength(rule.group)),
                      rule.format);
        }
    }
}
