#include "GitDiffView.hpp"
#include <QTextCursor>
#include <QTextCharFormat>
#include <QFont>
#include <QPalette>

namespace Kayte {

GitDiffView::GitDiffView(QWidget *parent) : QPlainTextEdit(parent) {
    setReadOnly(true);
    QFont f("monospace");
    f.setPointSize(10);
    setFont(f);

    QPalette p = palette();
    p.setColor(QPalette::Base, QColor(0x1E, 0x1E, 0x2E));
    p.setColor(QPalette::Text, QColor(0xCD, 0xD6, 0xF4));
    setPalette(p);
}

void GitDiffView::clear() { QPlainTextEdit::clear(); }

void GitDiffView::showDiff(const QVector<DiffLine> &lines) {
    QPlainTextEdit::clear();
    QTextCursor cur = textCursor();
    cur.beginEditBlock();

    for (const DiffLine &dl : lines) {
        QTextCharFormat fmt;
        switch (dl.origin) {
        case DiffLine::Origin::Addition:
            fmt.setBackground(QColor(0x1C, 0x40, 0x1C));
            fmt.setForeground(QColor(0xA6, 0xE3, 0xA1));
            break;
        case DiffLine::Origin::Deletion:
            fmt.setBackground(QColor(0x40, 0x1C, 0x1C));
            fmt.setForeground(QColor(0xF3, 0x8B, 0xA8));
            break;
        case DiffLine::Origin::FileHeader:
            fmt.setForeground(QColor(0x89, 0xDC, 0xEB));
            fmt.setFontWeight(QFont::Bold);
            break;
        case DiffLine::Origin::HunkHeader:
            fmt.setForeground(QColor(0xCB, 0xA6, 0xF7));
            break;
        default:
            fmt.setForeground(QColor(0xCD, 0xD6, 0xF4));
            break;
        }

        // Line number prefix for context / add / del lines
        QString linePrefix;
        if (dl.origin == DiffLine::Origin::Addition ||
            dl.origin == DiffLine::Origin::Deletion ||
            dl.origin == DiffLine::Origin::Context)
        {
            QString old = dl.oldLineNo > 0 ? QString::number(dl.oldLineNo).rightJustified(4) : "    ";
            QString nw  = dl.newLineNo > 0 ? QString::number(dl.newLineNo).rightJustified(4) : "    ";
            linePrefix  = old + " " + nw + " │ ";
        }

        cur.setCharFormat(fmt);
        cur.insertText(linePrefix + dl.content);
    }

    cur.endEditBlock();
    moveCursor(QTextCursor::Start);
}

} // namespace Kayte
