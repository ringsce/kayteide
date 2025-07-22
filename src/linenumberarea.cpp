// linenumberarea.cpp
#include "linenumberarea.h"
#include <QTextEdit>
#include <QPainter>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <QScrollBar> // <--- ADD THIS LINE to resolve the incomplete type error

LineNumberArea::LineNumberArea(QTextEdit *editor, QWidget *parent)
    : QWidget(parent)
    , codeEditor(editor)
{
    // Ensure this widget is always above the editor in z-order
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_StyledBackground); // Allow styling with stylesheets if needed
}

QSize LineNumberArea::sizeHint() const
{
    // Calculate width needed for line numbers
    // This is a basic calculation, for more precision, consider font metrics
    int digits = 1;
    int maxLineNumber = codeEditor->document()->blockCount();
    while (maxLineNumber >= 10) {
        maxLineNumber /= 10;
        digits++;
    }
    int width = 3 + digits * fontMetrics().horizontalAdvance(QLatin1Char('9')); // 3px padding
    return QSize(width, 0); // Height doesn't matter, it will be stretched by layout
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::lightGray); // Background for line number area

    QTextBlock block = codeEditor->document()->firstBlock();
    int lineNumber = 1;
    // Iterate through blocks and draw line numbers
    while (block.isValid()) {
        if (block.isVisible()) {
            QTextLayout *layout = block.layout();
            if (layout && !layout->formats().isEmpty()) { // Check if layout exists and has formats (i.e., non-empty block)
                // Get the top and bottom Y coordinates of the block in the editor's coordinate system
                QRectF blockRect = codeEditor->document()->documentLayout()->blockBoundingRect(block);

                // Convert block's Y-coordinate to the LineNumberArea's coordinate system
                // Adjust for editor's scroll bar position if present
                int top = qRound(blockRect.top() - codeEditor->verticalScrollBar()->value());
                int bottom = qRound(blockRect.bottom() - codeEditor->verticalScrollBar()->value());

                // Only draw if the block is visible within the event rect
                if (top <= event->rect().bottom() && bottom >= event->rect().top()) {
                    painter.setPen(Qt::darkGray); // Line number color
                    painter.drawText(0, top, width() - 3, fontMetrics().height(),
                                     Qt::AlignRight | Qt::AlignVCenter, QString::number(lineNumber));
                }
            }
        }
        block = block.next();
        lineNumber++;
    }
}