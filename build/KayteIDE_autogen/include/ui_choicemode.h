/********************************************************************************
** Form generated from reading UI file 'choicemode.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHOICEMODE_H
#define UI_CHOICEMODE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ChoiceMode
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QSpacerItem *verticalSpacer_2;
    QRadioButton *radioButtonTextMode;
    QRadioButton *radioButtonRADMode;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ChoiceMode)
    {
        if (ChoiceMode->objectName().isEmpty())
            ChoiceMode->setObjectName("ChoiceMode");
        ChoiceMode->resize(350, 200);
        verticalLayout = new QVBoxLayout(ChoiceMode);
        verticalLayout->setObjectName("verticalLayout");
        label = new QLabel(ChoiceMode);
        label->setObjectName("label");
        QFont font;
        font.setPointSize(14);
        font.setBold(true);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(label);

        verticalSpacer_2 = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        radioButtonTextMode = new QRadioButton(ChoiceMode);
        radioButtonTextMode->setObjectName("radioButtonTextMode");
        radioButtonTextMode->setChecked(true);

        verticalLayout->addWidget(radioButtonTextMode);

        radioButtonRADMode = new QRadioButton(ChoiceMode);
        radioButtonRADMode->setObjectName("radioButtonRADMode");

        verticalLayout->addWidget(radioButtonRADMode);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(ChoiceMode);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(ChoiceMode);

        QMetaObject::connectSlotsByName(ChoiceMode);
    } // setupUi

    void retranslateUi(QDialog *ChoiceMode)
    {
        ChoiceMode->setWindowTitle(QCoreApplication::translate("ChoiceMode", "Choose Development Mode", nullptr));
        label->setText(QCoreApplication::translate("ChoiceMode", "Choose Development Mode:", nullptr));
        radioButtonTextMode->setText(QCoreApplication::translate("ChoiceMode", "Text Editor Mode (Code Editor)", nullptr));
        radioButtonRADMode->setText(QCoreApplication::translate("ChoiceMode", "RAD (Rapid Application Development) Mode", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ChoiceMode: public Ui_ChoiceMode {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHOICEMODE_H
