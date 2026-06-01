#pragma once
// uicanvaswidget.h – KayteIDE
// Drop-target canvas: drag widgets from the palette, position them,
// then export to a standard Qt .ui XML file.

#include <QWidget>
#include <QList>
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QString>
#include <QMenu>
#include <QUndoStack>
#include <QUndoCommand>

// ─── One placed widget record ─────────────────────────────────────────────────
struct CanvasItem
{
    QString widgetType;   // "QPushButton", "QLabel", …
    QString objectName;   // "pushButton_1", "label_3", …
    QString labelText;    // displayed text / placeholder
    QRect   geometry;     // position + size on canvas
    bool    selected { false };
};

// ─── UiCanvasWidget ───────────────────────────────────────────────────────────
class UiCanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UiCanvasWidget(QWidget *parent = nullptr);

    // ── Mutation API (used by undo commands) ──────────────────────────────────
    void       insertItem(const CanvasItem &item);
    void       removeItem(int index);
    CanvasItem getItem   (int index) const;
    void       moveItem  (int index, QPoint topLeft);
    int        itemCount () const { return m_items.size(); }

    // ── User-facing ───────────────────────────────────────────────────────────
    bool       exportUiFile(const QString &filePath) const;
    void       clearCanvas();
    QUndoStack *undoStack() { return m_undoStack; }

signals:
    void itemSelectionChanged(const CanvasItem *item); // nullptr = nothing selected
    void canvasModified();

protected:
    void paintEvent        (QPaintEvent        *e) override;
    void dragEnterEvent    (QDragEnterEvent    *e) override;
    void dragMoveEvent     (QDragMoveEvent     *e) override;
    void dropEvent         (QDropEvent         *e) override;
    void mousePressEvent   (QMouseEvent        *e) override;
    void mouseMoveEvent    (QMouseEvent        *e) override;
    void mouseReleaseEvent (QMouseEvent        *e) override;
    void keyPressEvent     (QKeyEvent          *e) override;
    void contextMenuEvent  (QContextMenuEvent  *e) override;
    QSize sizeHint() const override { return { 640, 480 }; }

private:
    void  drawGrid            (QPainter &p) const;
    void  drawItem            (QPainter &p, const CanvasItem &item) const;
    void  drawSelectionHandles(QPainter &p, const CanvasItem &item) const;

    int   itemAt    (const QPoint &pos) const;
    void  selectItem(int index);
    void  deleteSelected();
    void  snapToGrid(QPoint &pt) const;

    QString uniqueName  (const QString &type) const;
    QString defaultLabel(const QString &type) const;

    QList<CanvasItem> m_items;
    int     m_selectedIndex  { -1 };
    QPoint  m_dragOffset;
    QPoint  m_moveStartPos;   // canvas-local top-left when a drag began (for undo)
    bool    m_draggingItem   { false };

    QPoint  m_dropPreviewPt;
    bool    m_showDropPreview{ false };
    QString m_dropPreviewType;

    static constexpr int GRID = 8;
    QUndoStack *m_undoStack  { nullptr };
};

// ─── Undo commands (defined inline – they're tiny) ────────────────────────────

class AddItemCmd : public QUndoCommand {
public:
    AddItemCmd(UiCanvasWidget *c, CanvasItem item)
        : QUndoCommand(QLatin1String("Add ") + item.widgetType)
        , m_c(c), m_item(std::move(item)) {}
    void redo() override { m_c->insertItem(m_item); }
    void undo() override { m_c->removeItem(m_c->itemCount() - 1); }
private:
    UiCanvasWidget *m_c;
    CanvasItem      m_item;
};

class DeleteItemCmd : public QUndoCommand {
public:
    DeleteItemCmd(UiCanvasWidget *c, int idx)
        : QUndoCommand(QLatin1String("Delete widget"))
        , m_c(c), m_idx(idx), m_item(c->getItem(idx)) {}
    void redo() override { m_c->removeItem(m_idx); }
    void undo() override { m_c->insertItem(m_item); }
private:
    UiCanvasWidget *m_c;
    int             m_idx;
    CanvasItem      m_item;
};

class MoveItemCmd : public QUndoCommand {
public:
    MoveItemCmd(UiCanvasWidget *c, int idx, QPoint from, QPoint to)
        : QUndoCommand(QLatin1String("Move widget"))
        , m_c(c), m_idx(idx), m_from(from), m_to(to) {}
    void redo() override { m_c->moveItem(m_idx, m_to); }
    void undo() override { m_c->moveItem(m_idx, m_from); }
private:
    UiCanvasWidget *m_c;
    int    m_idx;
    QPoint m_from, m_to;
};
