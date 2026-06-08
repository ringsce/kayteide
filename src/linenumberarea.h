#pragma once
#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

// ─────────────────────────────────────────────────────────────────────────────
// CodeEditor
//
// A thin QPlainTextEdit subclass that exposes five protected methods needed by
// LineNumberArea without any friend-class hacks:
//
//   firstVisibleBlock()       – first block visible in the viewport
//   blockBoundingGeometry()   – bounding rect in document coordinates
//   blockBoundingRect()       – bounding rect in block coordinates
//   contentOffset()           – scroll offset of the content
//   setViewportMargins()      – reserve space for the line-number gutter
//
// Use CodeEditor everywhere instead of QPlainTextEdit.  It is a drop-in
// replacement; no behaviour changes, only the five methods become public.
// ─────────────────────────────────────────────────────────────────────────────

#include <QPlainTextEdit>
#include <QWidget>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QSize>
#include <QRect>

// ── CodeEditor ────────────────────────────────────────────────────────────────
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr)
        : QPlainTextEdit(parent) {}

    // Expose the five protected QPlainTextEdit members as public.
    using QPlainTextEdit::firstVisibleBlock;
    using QPlainTextEdit::blockBoundingGeometry;
    using QPlainTextEdit::blockBoundingRect;
    using QPlainTextEdit::contentOffset;
    using QPlainTextEdit::setViewportMargins;
};

// ── LineNumberArea ────────────────────────────────────────────────────────────
// Works together with CodeEditor.  The uploaded linenumberarea.cpp implements
// the rendering using document()->documentLayout()->blockBoundingRect() which
// is fully public and does not need CodeEditor at all — so the .cpp file
// compiles unchanged.  This header only declares the class interface.
// ─────────────────────────────────────────────────────────────────────────────
class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    // Pass the CodeEditor (or any QPlainTextEdit) as editor.
    explicit LineNumberArea(QPlainTextEdit *editor, QWidget *parent = nullptr);

    // Swap the editor after construction (e.g. when reusing the widget).
    void setCodeEditor(QPlainTextEdit *editor);

    // Required width for the gutter given the current block count.
    QSize sizeHint() const override;

    // Colour customisation (all optional; defaults to light-grey gutter).
    void setBackgroundColor(const QColor &c) { m_bg = c; update(); }
    void setForegroundColor(const QColor &c) { m_fg = c; update(); }
    void setCurrentLineColor(const QColor &c){ m_currentFg = c; update(); }

    // Called by MainWindow::updateLineNumberAreaWidth to push a new margin.
    // Uses CodeEditor::setViewportMargins if the editor is a CodeEditor,
    // otherwise falls back to the document-layout width calculation.
    void updateWidth(int blockCount = 0);

protected:
    void paintEvent(QPaintEvent *event) override;

public slots:
    void setupConnections();
    void onDocumentChanged();
    void onBlockCountChanged(int newBlockCount);
    void updateArea(const QRect &rect, int dy);

private:
    QPlainTextEdit *m_codeEditor { nullptr };
    QColor          m_bg         { "#f0f0f0" };
    QColor          m_fg         { Qt::darkGray };
    QColor          m_currentFg  { Qt::black };
};

#endif // LINENUMBERAREA_H