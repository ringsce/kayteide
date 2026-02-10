#include "linenumberarea.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QDebug>

LineNumberArea::LineNumberArea(QPlainTextEdit *editor, QWidget *parent)
    : QWidget(parent)
    , m_codeEditor(editor)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_StyledBackground);

    // Initial setup of connections
    setupConnections();
}

void LineNumberArea::setupConnections()
{
    // Clear any existing connections to avoid duplicates
    disconnect(this);

    if (!m_codeEditor) {
        qWarning() << "LineNumberArea: No code editor set!";
        return;
    }

    // Connect to scroll bar signals
    if (m_codeEditor->verticalScrollBar()) {
        connect(m_codeEditor->verticalScrollBar(), &QScrollBar::valueChanged,
                this, QOverload<>::of(&QWidget::update));
        connect(m_codeEditor->verticalScrollBar(), &QScrollBar::rangeChanged,
                this, QOverload<>::of(&QWidget::update));
    }

    // Connect to document signals
    if (m_codeEditor->document()) {
        connect(m_codeEditor->document(), &QTextDocument::contentsChanged,
                this, &LineNumberArea::onDocumentChanged);
        connect(m_codeEditor, &QPlainTextEdit::blockCountChanged,
                this, &LineNumberArea::onBlockCountChanged);
        connect(m_codeEditor, &QPlainTextEdit::updateRequest,
                this, &LineNumberArea::updateArea);
    }

    // Force initial update
    update();
    updateGeometry();
}

void LineNumberArea::setCodeEditor(QPlainTextEdit *editor)
{
    if (m_codeEditor == editor) {
        return;
    }

    m_codeEditor = editor;
    setupConnections();

    qDebug() << "LineNumberArea: Editor set, block count:"
             << (m_codeEditor ? m_codeEditor->blockCount() : 0);
}

void LineNumberArea::onDocumentChanged()
{
    qDebug() << "LineNumberArea: Document changed, block count:"
             << (m_codeEditor ? m_codeEditor->blockCount() : 0);
    updateGeometry();
    update();
}

void LineNumberArea::onBlockCountChanged(int newBlockCount)
{
    qDebug() << "LineNumberArea: Block count changed to:" << newBlockCount;
    updateGeometry();
    update();
}

void LineNumberArea::updateArea(const QRect &rect, int dy)
{
    if (dy) {
        scroll(0, dy);
    } else {
        update(0, rect.y(), width(), rect.height());
    }

    if (rect.contains(m_codeEditor->viewport()->rect())) {
        updateGeometry();
    }
}

QSize LineNumberArea::sizeHint() const
{
    if (!m_codeEditor) {
        return QSize(0, 0);
    }

    int digits = 1;
    int maxBlock = m_codeEditor->blockCount();

    qDebug() << "LineNumberArea: sizeHint called, blockCount:" << maxBlock;

    if (maxBlock > 0) {
        digits = QString::number(maxBlock).length();
    }

    if (digits < 2) digits = 2;

    int width = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    width += 10; // Extra padding for better visual spacing

    return QSize(width, 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    if (!m_codeEditor || !m_codeEditor->document()) {
        qWarning() << "LineNumberArea: paintEvent called but no editor or document!";
        QPainter painter(this);
        painter.fillRect(event->rect(), Qt::lightGray);
        return;
    }

    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::lightGray); // Background for line number area

    QFont font = m_codeEditor->font();
    painter.setFont(font);
    painter.setPen(Qt::darkGray);

    // Get the current vertical scroll offset
    int scrollOffset = 0;
    if (m_codeEditor->verticalScrollBar()) {
        scrollOffset = m_codeEditor->verticalScrollBar()->value();
    }

    int blockCount = m_codeEditor->blockCount();
    qDebug() << "LineNumberArea: Painting" << blockCount << "blocks, scroll offset:" << scrollOffset;

    // Iterate through blocks starting from the first one in the document
    QTextBlock block = m_codeEditor->document()->firstBlock();
    int blockNumber = 0; // 0-indexed block number

    // Loop through blocks as long as they are valid
    while (block.isValid()) {
        // Get the block's bounding rectangle relative to the *document*
        QRectF blockRectInDocument = m_codeEditor->document()->documentLayout()->blockBoundingRect(block);

        // Calculate the block's top Y coordinate in the *viewport*
        int lineTopInViewport = qRound(blockRectInDocument.top() - scrollOffset);
        int lineHeight = qRound(blockRectInDocument.height());

        // Check if this line is visible within the current paint event's rectangle
        if (lineTopInViewport + lineHeight >= event->rect().top() &&
            lineTopInViewport <= event->rect().bottom()) {

            QString number = QString::number(blockNumber + 1); // 1-indexed line number

            int textHeight = painter.fontMetrics().height();
            // Calculate Y position to center text vertically within the line's bounding box
            int textY = lineTopInViewport + (lineHeight - textHeight) / 2 + painter.fontMetrics().ascent();

            painter.drawText(0, textY - painter.fontMetrics().ascent(),
                             width() - 5, // Leave a little padding from the right edge
                             textHeight,
                             Qt::AlignRight | Qt::AlignVCenter,
                             number);
        }

        // Optimization: If the current block's top is already past the paint event's bottom,
        // we can stop processing blocks
        if (lineTopInViewport > event->rect().bottom() && blockNumber > 0) {
            break;
        }

        block = block.next();
        blockNumber++;
    }
}