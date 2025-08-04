#include "linenumberarea.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QDebug> // Can be removed once everything is stable

LineNumberArea::LineNumberArea(QPlainTextEdit *editor, QWidget *parent)
    : QWidget(parent)
    , m_codeEditor(editor)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_StyledBackground);

    // Connect to signals from the editor's scroll bar and document changes
    // to trigger repaints of the line number area.
    if (m_codeEditor && m_codeEditor->verticalScrollBar()) {
        connect(m_codeEditor->verticalScrollBar(), &QScrollBar::valueChanged, this, QOverload<>::of(&QWidget::update));
        connect(m_codeEditor->verticalScrollBar(), &QScrollBar::rangeChanged, this, QOverload<>::of(&QWidget::update));
    }
    if (m_codeEditor && m_codeEditor->document()) {
        connect(m_codeEditor->document(), &QTextDocument::contentsChanged, this, QOverload<>::of(&QWidget::update));
        connect(m_codeEditor, &QPlainTextEdit::blockCountChanged, this, QOverload<>::of(&QWidget::update));
    }
}

QSize LineNumberArea::sizeHint() const
{
    if (!m_codeEditor) {
        return QSize(0, 0);
    }

    int digits = 1;
    int maxBlock = m_codeEditor->blockCount();
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
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::lightGray); // Background for line number area

    if (!m_codeEditor || !m_codeEditor->document()) {
        return;
    }

    QFont font = m_codeEditor->font();
    font.setPointSize(font.pointSize());
    painter.setFont(font);
    painter.setPen(Qt::darkGray);

    // Get the current vertical scroll offset using the public QScrollBar::value()
    int scrollOffset = m_codeEditor->verticalScrollBar()->value(); // <--- CORRECTED LINE

    // Iterate through blocks starting from the first one in the document
    QTextBlock block = m_codeEditor->document()->firstBlock();
    int blockNumber = 0; // 0-indexed block number

    // Loop through blocks as long as they are valid
    while (block.isValid()) {
        // Get the block's bounding rectangle relative to the *document*
        QRectF blockRectInDocument = m_codeEditor->document()->documentLayout()->blockBoundingRect(block);

        // Calculate the block's top Y coordinate in the *viewport*
        // by subtracting the scrollOffset (document_y - scroll_y = viewport_y).
        int lineTopInViewport = qRound(blockRectInDocument.top() - scrollOffset); // <--- CORRECTED LINE
        int lineHeight = qRound(blockRectInDocument.height());

        // Check if this line is visible within the current paint event's rectangle
        if (lineTopInViewport + lineHeight >= event->rect().top() && lineTopInViewport <= event->rect().bottom()) {
            QString number = QString::number(blockNumber + 1); // 1-indexed line number

            int textHeight = painter.fontMetrics().height();
            // Calculate Y position to center text vertically within the line's bounding box
            int textY = lineTopInViewport + (lineHeight - textHeight) / 2 + painter.fontMetrics().ascent();

            painter.drawText(0, textY - painter.fontMetrics().ascent(), // Draw from top of text bounds
                             width() - 5, // Leave a little padding from the right edge
                             textHeight,
                             Qt::AlignRight | Qt::AlignVCenter,
                             number);
        }

        // Optimization: If the current block's top (in viewport coordinates) is already past
        // the paint event's bottom, and we've processed at least one block, we can stop.
        // This is important because block.next() could jump far, and we don't need to process
        // blocks completely outside the paint area.
        if (lineTopInViewport > event->rect().bottom() && blockNumber > 0) {
            break;
        }

        block = block.next();
        blockNumber++;
    }
}