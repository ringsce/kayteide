#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSyntaxHighlighter>
#include <QTabWidget>
#include <QVector>

#include <QTreeView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QWidget>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include "../plugins/project/newprojectdialog.h" // Add this include

class LineNumberArea;
class EditorTabWidget;

#include "vbsyntaxhighlighter.h"
#include "cppsyntaxhighlighter.h"
#include "kaytesyntaxhighlighter.h"
#include "pascalsyntaxhighlighter.h"
#include "delphisyntaxhighlighter.h"
#include "choicemode.h"
#include "downloadprogressdialog.h"
#include "keyboard.h" // <--- ADD THIS INCLUDE

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void updateLineNumberArea(const QRect &rect, int dy);
    void closeEvent(QCloseEvent *event) override;


private slots:
    void on_actionNewFile_triggered();
    void handleSaveFileTriggered();
    bool handleSaveFileAsTriggered();
    void on_actionCloseTab_triggered();

    // Corrected / consolidated declarations:
    void handleOpenFileTriggered();
    void on_tabWidgetEditor_tabCloseRequested(int index); // Keep only ONE of these

    void handleTabModificationChanged(bool modified);
    void handleTabTitleChanged(const QString &title);

    void buildProject();
    void runProject();
    void cleanProject();
    void debugProject();

    void showAboutDialog();
    void updateLineNumberAreaWidth(int newBlockCount);

    void showModeSelectionDialog();
    void activateMode(ChoiceMode::DevelopmentMode mode);
    void handleDownloadDialogFinished();

    void updateTabTitle(bool modified);
    void updateTabTitleOnRename(const QString &newTitle);

    void handlePathLineEditReturnPressed();
    void handleListViewDoubleClicked(const QModelIndex &index);

    void saveProjectAs(); // <-- ADD THIS NEW SLOT

    void on_tabWidgetEditor_currentChanged(int index); // You will need a slot for tab changes

    // <--- ADD THIS NEW SLOT DECLARATION for the destroyed signal
    void onTabClosed(QObject* obj = nullptr); // Or just void onTabClosed(); if you don't need the QObject*

    void on_actionNewProject_triggered(); // Add this new slot


private:
    Ui::MainWindow *ui;

    QTreeView *fileListView;
    QFileSystemModel *fileSystemModel;
    QLineEdit *pathLineEdit;
    QPushButton *browseButton;

    void setupFileBrowser();
    void setCurrentPath(const QString &path);

    EditorTabWidget* currentEditorTab() const;
    void createNewTab(const QString &filePath = QString());
    bool saveCurrentFile();

    QString defaultDownloadPath;
    QStringList radModeRepos;
    QStringList editorModeRepos;
    void setupDownloadRepos();

    QVector<EditorTabWidget*> openEditorTabs; // Already exists

    // ADD THESE NEW MEMBERS to store project-specific settings if they don't exist
    // These will hold the current project's context
    QString m_currentProjectFilePath; // Full path to the .xprj file, if a project is loaded/saved
    QString m_currentProjectName;     // Name of the project (e.g., "MyProject")

    // Placeholders for build commands - initialize these elsewhere or link to your actual settings
    QString m_buildCommand = "make all"; // Example default
    QString m_runCommand = "./output_executable"; // Example default
    QString m_cleanCommand = "make clean"; // Example default
    QString m_debugCommand = "lldb ./output_executable"; // Example default

    ChoiceMode::DevelopmentMode currentDevelopmentMode;

    void populateProjectList();

    KeyboardShortcutsManager *m_keyboardShortcutsManager; // <--- ADD THIS MEMBER

};

#endif // MAINWINDOW_H