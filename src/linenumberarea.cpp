#include "linenumberarea.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QDebug>

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
    const int charW   = fontMetrics().horizontalAdvance(QLatin1Char('9'));
    const int newW    = 3 + charW * digits + 10;

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
    int w      = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 10;
    return { w, 0 };
}

// ─────────────────────────────────────────────────────────────────────────────
void LineNumberArea::paintEvent(QPaintEvent *event)
{
    if (!m_codeEditor || !m_codeEditor->document()) {
        QPainter painter(this);
        painter.fillRect(event->rect(), m_bg);
        return;
    }

    QPainter painter(this);
    painter.fillRect(event->rect(), m_bg);

    // Right border divider
    painter.setPen(QPen(m_fg.darker(130), 1));
    painter.drawLine(width() - 1, event->rect().top(),
                     width() - 1, event->rect().bottom());

    QFont  font = m_codeEditor->font();
    painter.setFont(font);

    // Current cursor block number for highlight
    const int cursorBlock = m_codeEditor->textCursor().blockNumber();

    // Scroll offset: use the public document-layout approach so we never
    // need firstVisibleBlock() or contentOffset() (both protected).
    const qreal scrollY   = m_codeEditor->verticalScrollBar()->value();
    const qreal docMargin = m_codeEditor->document()->documentMargin();
    const qreal offsetY   = -scrollY + docMargin;

    QTextBlock block      = m_codeEditor->document()->firstBlock();
    int        blockNum   = 0;

    while (block.isValid()) {
        const QRectF br   = m_codeEditor->document()
                            ->documentLayout()->blockBoundingRect(block);
        const int lineTop = qRound(br.top() + offsetY);
        const int lineH   = qRound(br.height());

        if (lineTop + lineH < event->rect().top()) {
            block = block.next();
            ++blockNum;
            continue;
        }
        if (lineTop > event->rect().bottom()) break;

        // Current-line gutter highlight
        if (blockNum == cursorBlock) {
            painter.fillRect(0, lineTop, width() - 1, lineH,
                             m_bg.lightness() < 128
                             ? m_bg.lighter(130)
                             : m_bg.darker(108));
            painter.setPen(m_currentFg);
        } else {
            painter.setPen(m_fg);
        }

        const QFontMetrics fm(font);
        const int textH = fm.height();
        const int textY = lineTop + (lineH - textH) / 2 + fm.ascent();

        painter.drawText(0, textY - fm.ascent(),
                         width() - 5, textH,
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(blockNum + 1));

        block = block.next();
        ++blockNum;
    }
}
