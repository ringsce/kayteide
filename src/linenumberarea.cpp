#include "linenumberarea.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QHelpEvent>
#include <QTextLayout>
#include <QToolTip>
#include "modebar.h"   // ModeBar::drawIcon — the same outline bug as the mode bar

// ─────────────────────────────────────────────────────────────────────────────
LineNumberArea::LineNumberArea(QPlainTextEdit *editor, QWidget *parent)
    : QWidget(parent)
    , m_codeEditor(editor)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_StyledBackground);
    setupConnections();
}

// ─────────────────────────────────────────────────────────────────────────────
void LineNumberArea::setupConnections()
{
    if (m_codeEditor) {
        if (m_codeEditor->verticalScrollBar())
            disconnect(m_codeEditor->verticalScrollBar(), nullptr, this, nullptr);
        if (m_codeEditor->document())
            disconnect(m_codeEditor->document(), nullptr, this, nullptr);
        disconnect(m_codeEditor, nullptr, this, nullptr);
    }

    if (!m_codeEditor) {
        qWarning() << "LineNumberArea: No code editor set!";
        return;
    }

    if (m_codeEditor->verticalScrollBar()) {
        connect(m_codeEditor->verticalScrollBar(), &QScrollBar::valueChanged,
                this, QOverload<>::of(&QWidget::update));
        connect(m_codeEditor->verticalScrollBar(), &QScrollBar::rangeChanged,
                this, QOverload<>::of(&QWidget::update));
    }

    if (m_codeEditor->document()) {
        connect(m_codeEditor->document(), &QTextDocument::contentsChanged,
                this, &LineNumberArea::onDocumentChanged);
        connect(m_codeEditor, &QPlainTextEdit::blockCountChanged,
                this, &LineNumberArea::onBlockCountChanged);
        connect(m_codeEditor, &QPlainTextEdit::updateRequest,
                this, &LineNumberArea::updateArea);
        // Repaint gutter when cursor moves so current-line highlight updates
        connect(m_codeEditor, &QPlainTextEdit::cursorPositionChanged,
                this, QOverload<>::of(&QWidget::update));
    }

    updateWidth(m_codeEditor ? m_codeEditor->blockCount() : 0);
    update();
    updateGeometry();
}

// ─────────────────────────────────────────────────────────────────────────────
void LineNumberArea::setCodeEditor(QPlainTextEdit *editor)
{
    if (m_codeEditor == editor) return;
    m_codeEditor = editor;
    setupConnections();
    qDebug() << "LineNumberArea: Editor set, block count:"
             << (m_codeEditor ? m_codeEditor->blockCount() : 0);
}

// ─────────────────────────────────────────────────────────────────────────────
void LineNumberArea::updateWidth(int blockCount)
{
    if (!m_codeEditor) return;

    const int bc      = blockCount > 0 ? blockCount : m_codeEditor->blockCount();
    int       digits  = qMax(2, QString::number(qMax(1, bc)).length());
    const int charW   = QFontMetrics(m_codeEditor->font()).horizontalAdvance(QLatin1Char('9'));
    const int newW    = kIconColumn + 3 + charW * digits + 10;

    // setViewportMargins is protected on QPlainTextEdit.
    // If the editor is a CodeEditor we can call it directly via the using-declaration.
    // Otherwise fall back to doing nothing (margins stay at whatever they were).
    if (auto *ce = qobject_cast<CodeEditor *>(m_codeEditor)) {
        ce->setViewportMargins(newW, 0, 0, 0);
    }

    if (newW != width()) {
        resize(newW, height());
        updateGeometry();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void LineNumberArea::onDocumentChanged()
{
    updateWidth(m_codeEditor ? m_codeEditor->blockCount() : 0);
    update();
    updateGeometry();
}

void LineNumberArea::onBlockCountChanged(int newBlockCount)
{
    updateWidth(newBlockCount);
    update();
    updateGeometry();
}

void LineNumberArea::updateArea(const QRect &rect, int dy)
{
    if (dy)
        scroll(0, dy);
    else
        update(0, rect.y(), width(), rect.height());

    if (m_codeEditor && rect.contains(m_codeEditor->viewport()->rect()))
        updateWidth(m_codeEditor->blockCount());
}

// ─────────────────────────────────────────────────────────────────────────────
QSize LineNumberArea::sizeHint() const
{
    if (!m_codeEditor) return { 0, 0 };

    int digits = qMax(2, QString::number(qMax(1, m_codeEditor->blockCount())).length());
    int w      = kIconColumn + 3 + QFontMetrics(m_codeEditor->font()).horizontalAdvance(QLatin1Char('9')) * digits + 10;
    return { w, 0 };
}

// ─────────────────────────────────────────────────────────────────────────────
void LineNumberArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), m_bg);
    if (!m_codeEditor || !m_codeEditor->document()) return;

    painter.setPen(QPen(m_bg.lightness() < 128 ? m_bg.lighter(140) : m_bg.darker(115), 1));
    painter.drawLine(width() - 1, event->rect().top(), width() - 1, event->rect().bottom());

    const QFont font = m_codeEditor->font();
    painter.setFont(font);
    const QFontMetrics fm(font);

    // cursorRect() is in viewport coordinates; map them into ours, so the
    // numbers line up whether the gutter sits beside the editor or inside it.
    const int yOffset = m_codeEditor->viewport()->mapTo(window(), QPoint(0, 0)).y()
                      - mapTo(window(), QPoint(0, 0)).y();
    const int viewportH  = m_codeEditor->viewport()->height();
    const int cursorLine = m_codeEditor->textCursor().blockNumber();

    // Start at the first visible line and walk down until we leave the view.
    QTextBlock block = m_codeEditor->cursorForPosition(QPoint(0, 0)).block();
    while (block.isValid()) {
        const QRect lineRect = m_codeEditor->cursorRect(QTextCursor(block));
        if (lineRect.top() > viewportH) break;
        if (block.isVisible()) {
            // A wrapped line spans several rows; its number sits on the first.
            const int top = lineRect.top() + yOffset;
            const int h   = qMax(lineRect.height(), fm.height());
            if (top + h >= event->rect().top() && top <= event->rect().bottom()) {
                if (block.blockNumber() == cursorLine) {
                    painter.fillRect(0, top, width() - 1, h,
                                     m_bg.lightness() < 128 ? m_bg.lighter(130) : m_bg.darker(108));
                    painter.setPen(m_currentFg);
                } else {
                    painter.setPen(m_fg);
                }
                painter.drawText(0, top, width() - 6, h, Qt::AlignRight | Qt::AlignVCenter,
                                 QString::number(block.blockNumber() + 1));
                if (const auto it = m_markers.constFind(block.blockNumber()); it != m_markers.cend()) {
                    const int s = qMin(14, h);
                    const QColor c = it->error ? QColor(0xe5, 0x48, 0x4d) : QColor(0xd9, 0xa4, 0x00);
                    painter.save();
                    ModeBar::drawIcon(painter, ModeBar::Debug, QRectF(1, top + (h - s) / 2.0, s, s), c, m_bg);
                    painter.restore();
                }
            }
        }
        block = block.next();
    }
}

void LineNumberArea::setMarkers(const QMap<int, Marker> &byBlockNumber)
{
    m_markers = byBlockNumber;
    update();
}

int LineNumberArea::blockAt(int y) const
{
    if (!m_codeEditor) return -1;
    const int yOffset = m_codeEditor->viewport()->mapTo(window(), QPoint(0, 0)).y()
                      - mapTo(window(), QPoint(0, 0)).y();
    QTextBlock block = m_codeEditor->cursorForPosition(QPoint(0, 0)).block();
    for (; block.isValid(); block = block.next()) {
        const QRect r = m_codeEditor->cursorRect(QTextCursor(block));
        const int top = r.top() + yOffset;
        if (top > height()) break;
        const int bottom = top + qRound(block.layout()->boundingRect().height());
        if (y >= top && y < qMax(bottom, top + r.height())) return block.blockNumber();
    }
    return -1;
}

bool LineNumberArea::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        auto *he = static_cast<QHelpEvent *>(e);
        const auto it = m_markers.constFind(blockAt(he->pos().y()));
        if (it != m_markers.cend())
            QToolTip::showText(he->globalPos(), it->text, this);
        else
            QToolTip::hideText();
        return true;
    }
    return QWidget::event(e);
}
