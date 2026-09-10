#pragma once
// componentseditordock.h – KayteIDE
// Left-side dock: lists the components currently placed on the UiCanvasWidget
// (the active form), kept in sync with canvas selection/edits.

#include <QDockWidget>
#include <QListWidget>

class UiCanvasWidget;
struct CanvasItem;

class ComponentsEditorDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit ComponentsEditorDock(UiCanvasWidget *canvas, QWidget *parent = nullptr);

private slots:
    void refreshList();
    void onCanvasSelectionChanged(const CanvasItem *item);
    void onRowActivated(int row);

private:
    UiCanvasWidget *m_canvas   { nullptr };
    QListWidget    *m_list     { nullptr };
    bool            m_syncing  { false }; // guards against selection feedback loops
};
