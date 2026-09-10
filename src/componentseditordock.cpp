// componentseditordock.cpp – KayteIDE
#include "componentseditordock.h"
#include "uicanvaswidget.h"

#include <QVBoxLayout>
#include <QListWidgetItem>

ComponentsEditorDock::ComponentsEditorDock(UiCanvasWidget *canvas, QWidget *parent)
    : QDockWidget(tr("Components"), parent)
    , m_canvas(canvas)
{
    setObjectName(QStringLiteral("ComponentsEditorDock"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto *container = new QWidget(this);
    auto *lay = new QVBoxLayout(container);
    lay->setContentsMargins(4, 4, 4, 4);

    m_list = new QListWidget(container);
    m_list->setAlternatingRowColors(true);
    lay->addWidget(m_list);
    setWidget(container);

    connect(m_list, &QListWidget::currentRowChanged,
            this, &ComponentsEditorDock::onRowActivated);

    connect(m_canvas, &UiCanvasWidget::canvasModified,
            this, &ComponentsEditorDock::refreshList);
    connect(m_canvas, &UiCanvasWidget::itemSelectionChanged,
            this, &ComponentsEditorDock::onCanvasSelectionChanged);

    refreshList();
}

void ComponentsEditorDock::refreshList()
{
    m_syncing = true;
    m_list->clear();
    for (const CanvasItem &ci : m_canvas->items()) {
        auto *it = new QListWidgetItem(
            QStringLiteral("%1  [%2]").arg(ci.objectName, ci.widgetType));
        m_list->addItem(it);
    }
    m_list->setCurrentRow(m_canvas->selectedIndex());
    m_syncing = false;
}

void ComponentsEditorDock::onCanvasSelectionChanged(const CanvasItem *)
{
    if (m_syncing) return;
    m_syncing = true;
    m_list->setCurrentRow(m_canvas->selectedIndex());
    m_syncing = false;
}

void ComponentsEditorDock::onRowActivated(int row)
{
    if (m_syncing) return;
    m_canvas->selectItemExternally(row);
}
