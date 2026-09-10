#pragma once
// propertyeditordock.h – KayteIDE
// Right-side dock: edits the properties (name, text, geometry) of whichever
// component is currently selected on the UiCanvasWidget.

#include <QDockWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>

class UiCanvasWidget;
struct CanvasItem;

class PropertyEditorDock : public QDockWidget
{
    Q_OBJECT
public:
    explicit PropertyEditorDock(UiCanvasWidget *canvas, QWidget *parent = nullptr);

private slots:
    void onCanvasSelectionChanged(const CanvasItem *item);
    void applyChanges();

private:
    void setFieldsEnabled(bool enabled);

    UiCanvasWidget *m_canvas       { nullptr };
    int             m_currentIndex{ -1 };
    bool            m_updating    { false }; // guards against edit feedback loops

    QLabel    *m_typeValue { nullptr };
    QLineEdit *m_nameEdit  { nullptr };
    QLineEdit *m_textEdit  { nullptr };
    QSpinBox  *m_xSpin     { nullptr };
    QSpinBox  *m_ySpin     { nullptr };
    QSpinBox  *m_wSpin     { nullptr };
    QSpinBox  *m_hSpin     { nullptr };
};
