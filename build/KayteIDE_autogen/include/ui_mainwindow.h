/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionOpen;
    QAction *actionSave;
    QAction *actionSave_As;
    QAction *actionSaveProjectAs;
    QAction *actionExit;
    QAction *actionBuild;
    QAction *actionClean;
    QAction *actionRun;
    QAction *actionDebug;
    QAction *actionAbout;
    QAction *actionNewFile;
    QAction *actionCloseTab;
    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
    QAction *actionSelectAll;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QStackedWidget *centralStackedWidget;
    QWidget *editorPage;
    QVBoxLayout *editorPageLayout;
    QTabWidget *tabWidgetEditor;
    QWidget *projectPage;
    QVBoxLayout *projectPageLayout;
    QListWidget *projectListWidget;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuProject;
    QMenu *menuHelp;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        actionOpen = new QAction(MainWindow);
        actionOpen->setObjectName("actionOpen");
        actionSave = new QAction(MainWindow);
        actionSave->setObjectName("actionSave");
        actionSave_As = new QAction(MainWindow);
        actionSave_As->setObjectName("actionSave_As");
        actionSaveProjectAs = new QAction(MainWindow);
        actionSaveProjectAs->setObjectName("actionSaveProjectAs");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionBuild = new QAction(MainWindow);
        actionBuild->setObjectName("actionBuild");
        actionClean = new QAction(MainWindow);
        actionClean->setObjectName("actionClean");
        actionRun = new QAction(MainWindow);
        actionRun->setObjectName("actionRun");
        actionDebug = new QAction(MainWindow);
        actionDebug->setObjectName("actionDebug");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        actionNewFile = new QAction(MainWindow);
        actionNewFile->setObjectName("actionNewFile");
        actionCloseTab = new QAction(MainWindow);
        actionCloseTab->setObjectName("actionCloseTab");
        actionCut = new QAction(MainWindow);
        actionCut->setObjectName("actionCut");
        actionCopy = new QAction(MainWindow);
        actionCopy->setObjectName("actionCopy");
        actionPaste = new QAction(MainWindow);
        actionPaste->setObjectName("actionPaste");
        actionSelectAll = new QAction(MainWindow);
        actionSelectAll->setObjectName("actionSelectAll");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        centralStackedWidget = new QStackedWidget(centralwidget);
        centralStackedWidget->setObjectName("centralStackedWidget");
        editorPage = new QWidget();
        editorPage->setObjectName("editorPage");
        editorPageLayout = new QVBoxLayout(editorPage);
        editorPageLayout->setObjectName("editorPageLayout");
        tabWidgetEditor = new QTabWidget(editorPage);
        tabWidgetEditor->setObjectName("tabWidgetEditor");
        tabWidgetEditor->setTabsClosable(true);
        tabWidgetEditor->setMovable(true);

        editorPageLayout->addWidget(tabWidgetEditor);

        centralStackedWidget->addWidget(editorPage);
        projectPage = new QWidget();
        projectPage->setObjectName("projectPage");
        projectPageLayout = new QVBoxLayout(projectPage);
        projectPageLayout->setObjectName("projectPageLayout");
        projectListWidget = new QListWidget(projectPage);
        projectListWidget->setObjectName("projectListWidget");
        QFont font;
        font.setPointSize(12);
        projectListWidget->setFont(font);
        projectListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

        projectPageLayout->addWidget(projectListWidget);

        centralStackedWidget->addWidget(projectPage);

        verticalLayout->addWidget(centralStackedWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 24));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName("menuEdit");
        menuProject = new QMenu(menubar);
        menuProject->setObjectName("menuProject");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName("toolBar");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuProject->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionNewFile);
        menuFile->addAction(actionOpen);
        menuFile->addAction(actionSave);
        menuFile->addAction(actionSave_As);
        menuFile->addAction(actionSaveProjectAs);
        menuFile->addSeparator();
        menuFile->addAction(actionCloseTab);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuEdit->addAction(actionCut);
        menuEdit->addAction(actionCopy);
        menuEdit->addAction(actionPaste);
        menuEdit->addSeparator();
        menuEdit->addAction(actionSelectAll);
        menuProject->addAction(actionBuild);
        menuProject->addAction(actionClean);
        menuProject->addSeparator();
        menuProject->addAction(actionRun);
        menuProject->addAction(actionDebug);
        menuHelp->addAction(actionAbout);
        toolBar->addAction(actionNewFile);
        toolBar->addAction(actionOpen);
        toolBar->addAction(actionSave);
        toolBar->addAction(actionSave_As);
        toolBar->addAction(actionSaveProjectAs);
        toolBar->addAction(actionCloseTab);
        toolBar->addSeparator();
        toolBar->addAction(actionCut);
        toolBar->addAction(actionCopy);
        toolBar->addAction(actionPaste);
        toolBar->addAction(actionSelectAll);
        toolBar->addSeparator();
        toolBar->addAction(actionBuild);
        toolBar->addAction(actionClean);
        toolBar->addAction(actionRun);
        toolBar->addAction(actionDebug);
        toolBar->addSeparator();
        toolBar->addAction(actionAbout);
        toolBar->addAction(actionExit);

        retranslateUi(MainWindow);

        tabWidgetEditor->setCurrentIndex(-1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Kayte IDE", nullptr));
        actionOpen->setText(QCoreApplication::translate("MainWindow", "&Open", nullptr));
#if QT_CONFIG(shortcut)
        actionOpen->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        actionSave->setText(QCoreApplication::translate("MainWindow", "&Save", nullptr));
#if QT_CONFIG(shortcut)
        actionSave->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionSave_As->setText(QCoreApplication::translate("MainWindow", "Save &As...", nullptr));
#if QT_CONFIG(shortcut)
        actionSave_As->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Shift+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionSaveProjectAs->setText(QCoreApplication::translate("MainWindow", "Save Project &As...", nullptr));
#if QT_CONFIG(shortcut)
        actionSaveProjectAs->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Alt+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExit->setText(QCoreApplication::translate("MainWindow", "E&xit", nullptr));
        actionBuild->setText(QCoreApplication::translate("MainWindow", "&Build", nullptr));
#if QT_CONFIG(tooltip)
        actionBuild->setToolTip(QCoreApplication::translate("MainWindow", "Build Project", nullptr));
#endif // QT_CONFIG(tooltip)
        actionClean->setText(QCoreApplication::translate("MainWindow", "&Clean", nullptr));
#if QT_CONFIG(tooltip)
        actionClean->setToolTip(QCoreApplication::translate("MainWindow", "Clean Project", nullptr));
#endif // QT_CONFIG(tooltip)
        actionRun->setText(QCoreApplication::translate("MainWindow", "&Run", nullptr));
#if QT_CONFIG(tooltip)
        actionRun->setToolTip(QCoreApplication::translate("MainWindow", "Run Project", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionRun->setShortcut(QCoreApplication::translate("MainWindow", "F5", nullptr));
#endif // QT_CONFIG(shortcut)
        actionDebug->setText(QCoreApplication::translate("MainWindow", "&Debug", nullptr));
#if QT_CONFIG(tooltip)
        actionDebug->setToolTip(QCoreApplication::translate("MainWindow", "Debug Project", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionDebug->setShortcut(QCoreApplication::translate("MainWindow", "F9", nullptr));
#endif // QT_CONFIG(shortcut)
        actionAbout->setText(QCoreApplication::translate("MainWindow", "&About KayteIDE...", nullptr));
#if QT_CONFIG(tooltip)
        actionAbout->setToolTip(QCoreApplication::translate("MainWindow", "About KayteIDE", nullptr));
#endif // QT_CONFIG(tooltip)
        actionNewFile->setText(QCoreApplication::translate("MainWindow", "&New File", nullptr));
#if QT_CONFIG(shortcut)
        actionNewFile->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+N", nullptr));
#endif // QT_CONFIG(shortcut)
        actionCloseTab->setText(QCoreApplication::translate("MainWindow", "&Close Tab", nullptr));
#if QT_CONFIG(shortcut)
        actionCloseTab->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+W", nullptr));
#endif // QT_CONFIG(shortcut)
        actionCut->setText(QCoreApplication::translate("MainWindow", "Cu&t", nullptr));
#if QT_CONFIG(shortcut)
        actionCut->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+X", nullptr));
#endif // QT_CONFIG(shortcut)
        actionCopy->setText(QCoreApplication::translate("MainWindow", "&Copy", nullptr));
#if QT_CONFIG(shortcut)
        actionCopy->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+C", nullptr));
#endif // QT_CONFIG(shortcut)
        actionPaste->setText(QCoreApplication::translate("MainWindow", "&Paste", nullptr));
#if QT_CONFIG(shortcut)
        actionPaste->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+V", nullptr));
#endif // QT_CONFIG(shortcut)
        actionSelectAll->setText(QCoreApplication::translate("MainWindow", "Select &All", nullptr));
#if QT_CONFIG(shortcut)
        actionSelectAll->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+A", nullptr));
#endif // QT_CONFIG(shortcut)
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "&File", nullptr));
        menuEdit->setTitle(QCoreApplication::translate("MainWindow", "&Edit", nullptr));
        menuProject->setTitle(QCoreApplication::translate("MainWindow", "&Project", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "&Help", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
