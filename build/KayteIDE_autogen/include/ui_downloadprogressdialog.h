/********************************************************************************
** Form generated from reading UI file 'downloadprogressdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DOWNLOADPROGRESSDIALOG_H
#define UI_DOWNLOADPROGRESSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_DownloadProgressDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *statusLabel;
    QProgressBar *progressBarOverall;
    QListWidget *repoListWidget;
    QTextEdit *logTextEdit;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *DownloadProgressDialog)
    {
        if (DownloadProgressDialog->objectName().isEmpty())
            DownloadProgressDialog->setObjectName("DownloadProgressDialog");
        DownloadProgressDialog->resize(600, 450);
        verticalLayout = new QVBoxLayout(DownloadProgressDialog);
        verticalLayout->setObjectName("verticalLayout");
        statusLabel = new QLabel(DownloadProgressDialog);
        statusLabel->setObjectName("statusLabel");
        QFont font;
        font.setPointSize(14);
        font.setBold(true);
        statusLabel->setFont(font);
        statusLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(statusLabel);

        progressBarOverall = new QProgressBar(DownloadProgressDialog);
        progressBarOverall->setObjectName("progressBarOverall");
        progressBarOverall->setValue(0);

        verticalLayout->addWidget(progressBarOverall);

        repoListWidget = new QListWidget(DownloadProgressDialog);
        repoListWidget->setObjectName("repoListWidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(repoListWidget->sizePolicy().hasHeightForWidth());
        repoListWidget->setSizePolicy(sizePolicy);
        repoListWidget->setSelectionMode(QAbstractItemView::NoSelection);

        verticalLayout->addWidget(repoListWidget);

        logTextEdit = new QTextEdit(DownloadProgressDialog);
        logTextEdit->setObjectName("logTextEdit");
        logTextEdit->setReadOnly(true);
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Monospace")});
        font1.setPointSize(10);
        logTextEdit->setFont(font1);

        verticalLayout->addWidget(logTextEdit);

        buttonBox = new QDialogButtonBox(DownloadProgressDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(DownloadProgressDialog);

        QMetaObject::connectSlotsByName(DownloadProgressDialog);
    } // setupUi

    void retranslateUi(QDialog *DownloadProgressDialog)
    {
        DownloadProgressDialog->setWindowTitle(QCoreApplication::translate("DownloadProgressDialog", "Initializing Kayte IDE - Downloading Resources", nullptr));
        statusLabel->setText(QCoreApplication::translate("DownloadProgressDialog", "Preparing to download...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DownloadProgressDialog: public Ui_DownloadProgressDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DOWNLOADPROGRESSDIALOG_H
