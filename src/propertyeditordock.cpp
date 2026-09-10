// propertyeditordock.cpp – KayteIDE
#include "propertyeditordock.h"
#include "uicanvaswidget.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>

PropertyEditorDock::PropertyEditorDock(UiCanvasWidget *canvas, QWidget *parent)
    : QDockWidget(tr("Properties"), parent)
    , m_canvas(canvas)
{
    setObjectName(QStringLiteral("PropertyEditorDock"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto *container = new QWidget(this);
    auto *outer = new QVBoxLayout(container);
    outer->setContentsMargins(6, 6, 6, 6);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);

    m_typeValue = new QLabel(QStringLiteral("—"), container);
    m_nameEdit  = new QLineEdit(container);
    m_textEdit  = new QLineEdit(container);

    m_xSpin = new QSpinBox(container); m_xSpin->setRange(0, 100000);
    m_ySpin = new QSpinBox(container); m_ySpin->setRange(0, 100000);
    m_wSpin = new QSpinBox(container); m_wSpin->setRange(1, 100000);
    m_hSpin = new QSpinBox(container); m_hSpin->setRange(1, 100000);

    auto *posLay = new QHBoxLayout;
    posLay->addWidget(new QLabel(tr("X:"), container)); posLay->addWidget(m_xSpin);
    posLay->addWidget(new QLabel(tr("Y:"), container)); posLay->addWidget(m_ySpin);

    auto *sizeLay = new QHBoxLayout;
    sizeLay->addWidget(new QLabel(tr("W:"), container)); sizeLay->addWidget(m_wSpin);
    sizeLay->addWidget(new QLabel(tr("H:"), container)); sizeLay->addWidget(m_hSpin);

    form->addRow(tr("Type"), m_typeValue);
    form->addRow(tr("Name"), m_nameEdit);
    form->addRow(tr("Text"), m_textEdit);
    form->addRow(tr("Position"), posLay);
    form->addRow(tr("Size"), sizeLay);

    outer->addLayout(form);
    outer->addStretch();
    setWidget(container);

    connect(m_nameEdit, &QLineEdit::editingFinished, this, &PropertyEditorDock::applyChanges);
    connect(m_textEdit, &QLineEdit::editingFinished, this, &PropertyEditorDock::applyChanges);
    connect(m_xSpin, &QSpinBox::editingFinished, this, &PropertyEditorDock::applyChanges);
    connect(m_ySpin, &QSpinBox::editingFinished, this, &PropertyEditorDock::applyChanges);
    connect(m_wSpin, &QSpinBox::editingFinished, this, &PropertyEditorDock::applyChanges);
    connect(m_hSpin, &QSpinBox::editingFinished, this, &PropertyEditorDock::applyChanges);

    connect(m_canvas, &UiCanvasWidget::itemSelectionChanged,
            this, &PropertyEditorDock::onCanvasSelectionChanged);

    setFieldsEnabled(false);
}

void PropertyEditorDock::setFieldsEnabled(bool enabled)
{
    m_nameEdit->setEnabled(enabled);
    m_textEdit->setEnabled(enabled);
    m_xSpin->setEnabled(enabled);
    m_ySpin->setEnabled(enabled);
    m_wSpin->setEnabled(enabled);
    m_hSpin->setEnabled(enabled);
}

void PropertyEditorDock::onCanvasSelectionChanged(const CanvasItem *item)
{
    m_currentIndex = m_canvas->selectedIndex();
    m_updating = true;
    if (!item) {
        m_typeValue->setText(QStringLiteral("—"));
        m_nameEdit->clear();
        m_textEdit->clear();
        m_xSpin->setValue(0); m_ySpin->setValue(0);
        m_wSpin->setValue(1); m_hSpin->setValue(1);
        setFieldsEnabled(false);
    } else {
        m_typeValue->setText(item->widgetType);
        m_nameEdit->setText(item->objectName);
        m_textEdit->setText(item->labelText);
        m_xSpin->setValue(item->geometry.x());
        m_ySpin->setValue(item->geometry.y());
        m_wSpin->setValue(item->geometry.width());
        m_hSpin->setValue(item->geometry.height());
        setFieldsEnabled(true);
    }
    m_updating = false;
}

void PropertyEditorDock::applyChanges()
{
    if (m_updating || m_currentIndex < 0) return;

    const QRect geo(m_xSpin->value(), m_ySpin->value(),
                     m_wSpin->value(), m_hSpin->value());

    m_updating = true; // the resulting itemSelectionChanged echo shouldn't re-apply
    m_canvas->applyItemEdits(m_currentIndex, m_nameEdit->text(), m_textEdit->text(), geo);
    m_updating = false;
}
