#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSyntaxHighlighter> // Base class for highlighters
#include <QTabWidget> // NEW: Include QTabWidget for the tabbed interface
#include <QVector>    // NEW: To manage multiple editor tabs

// Forward declarations for custom widgets/classes
class LineNumberArea;
class EditorTabWidget; // NEW: Forward declaration for your custom editor widget

// ADD THESE INCLUDES
#include "vbsyntaxhighlighter.h"
#include "cppsyntaxhighlighter.h"
#include "kaytesyntaxhighlighter.h"
#include "pascalsyntaxhighlighter.h"
#include "delphisyntaxhighlighter.h"
#include "choicemode.h"        // Needed for ChoiceMode::DevelopmentMode in activateMode
#include "downloadprogressdialog.h" // Needed for DownloadProgressDialog


// Include the namespace for Ui::MainWindow
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // REMOVED: enum DevelopmentMode { TextEditor, RAD };
    // This enum is now solely defined and used via ChoiceMode::DevelopmentMode

protected:
    void resizeEvent(QResizeEvent *event) override;
    // For LineNumberArea positioning and repainting (now specific to current tab, or moved to EditorTabWidget)
    // You might move this into EditorTabWidget if lineNumberArea is per-tab.
    void updateLineNumberArea(const QRect &rect, int dy);
    // NEW: Override closeEvent to handle unsaved changes across tabs
    void closeEvent(QCloseEvent *event) override;


private slots:
    // NEW FILE/OPEN/SAVE actions for the tabbed interface
    void on_actionNewFile_triggered();   // Connect to a "New File" QAction
    void on_actionOpenFile_triggered();  // Connect to an "Open File" QAction
    void on_actionSaveFile_triggered();  // Connect to a "Save" QAction
    bool on_actionSaveFileAs_triggered(); // Connect to a "Save As" QAction
    void on_actionCloseTab_triggered();  // Connect to a "Close Tab" QAction

    // Slot for when a tab's close button (the 'x') is clicked
    void on_tabWidgetEditor_tabCloseRequested(int index);

    // Existing project actions (these might operate on the current tab's content, or project files)
    void buildProject();
    void runProject();
    void cleanProject();
    void debugProject();

    void showAboutDialog();

    // Slot to update LineNumberArea width (might be moved to EditorTabWidget)
    void updateLineNumberAreaWidth(int newBlockCount);

    // Mode selection and download related slots
    void showModeSelectionDialog();
    // No longer onModeSelected, as the ChoiceMode dialog will call activateMode directly
    // void onModeSelected(ChoiceMode::DevelopmentMode mode); // This slot can be removed if activateMode is sufficient

    // CHANGED: Parameter type from 'int' to 'ChoiceMode::DevelopmentMode'
    void activateMode(ChoiceMode::DevelopmentMode mode);
    void handleDownloadDialogFinished(); // New slot to handle dialog completion

    // NEW: Slots to update tab title based on modification state or filename changes
    void updateTabTitle(bool modified);
    void updateTabTitleOnRename(const QString &newTitle);

private:
    Ui::MainWindow *ui;
    // QString currentFile; // NO LONGER NEEDED, each tab manages its own file path

    // Syntax highlighters - These should ideally be managed *per EditorTabWidget* now.
    // If you intend to reuse them, they would be members of EditorTabWidget.
    // currentHighlighter;
    // vbHighlighter;
    // cppHighlighter;
    // kayteHighlighter;
    // pascalHighlighter;
    // delphiHighlighter;
    // These specific highlighters should likely be moved to EditorTabWidget

    // Line number area - Also should be managed *per EditorTabWidget*
    // LineNumberArea *lineNumberArea;

    // Helper to set the active syntax highlighter - Also should be in EditorTabWidget
    // void setHighlighter(QSyntaxHighlighter *newHighlighter);

    // NEW: List to keep track of all opened EditorTabWidget instances
    QVector<EditorTabWidget*> openEditorTabs;
    // NEW: Helper to get the currently active editor tab
    EditorTabWidget* currentEditorTab() const;
    // NEW: Helper to create a new editor tab (for new file or opening existing)
    void createNewTab(const QString &filePath = QString());
    // NEW: Helper to save the current tab's file, prompting user if modified
    bool saveCurrentFile();

    // Download Management
    QString defaultDownloadPath;
    QStringList radModeRepos;
    QStringList editorModeRepos;
    void setupDownloadRepos(); // Helper to populate repo lists

    // CHANGED: Type from 'DevelopmentMode' to 'ChoiceMode::DevelopmentMode'
    ChoiceMode::DevelopmentMode currentDevelopmentMode;
};

#endif // MAINWINDOW_H