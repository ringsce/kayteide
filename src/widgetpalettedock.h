#pragma once
// widgetpalettedock.h – KayteIDE
// Left-side dock: categorised Qt6 widget palette for the visual .ui designer.
// Each item is draggable onto the UiCanvasWidget.

#include <QDockWidget>
#include <QToolBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QLabel>
#include <QFont>

// ─── Drag-enabled list widget ─────────────────────────────────────────────────
// Each item carries a "widgetType" string in the drag MIME data so the canvas
// knows what to instantiate.
class PaletteListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit PaletteListWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint m_dragStartPos;
};

// ─── WidgetPaletteDock ────────────────────────────────────────────────────────
class WidgetPaletteDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit WidgetPaletteDock(const QFont &faFont, QWidget *parent = nullptr);

private:
    void buildPalette();

    // Helper: add a category with a list of {displayName, widgetType, faGlyph}
    struct PaletteItem { QString label; QString type; const char *icon; };
    PaletteListWidget *addCategory(const QString &title,
                                   const char    *titleIcon,
                                   const QList<PaletteItem> &items);

    QToolBox  *m_toolBox    { nullptr };
    QLineEdit *m_searchEdit { nullptr };
    QFont      m_faFont;

private slots:
    void onSearchChanged(const QString &text);
};
