#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
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
#include <QProcess>
#include <QPlainTextEdit>
#include <QFont>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QToolButton>
#include <QMenu>
#include <QAction>

#include "../plugins/project/newprojectdialog.h"
#include "GitClientPanel.hpp"
#include "widgetpalettedock.h"
#include "uicanvaswidget.h"


class LineNumberArea;
class EditorTabWidget;

#include "vbsyntaxhighlighter.h"
#include "cppsyntaxhighlighter.h"
#include "kaytesyntaxhighlighter.h"
#include "pascalsyntaxhighlighter.h"
#include "delphisyntaxhighlighter.h"
#include "choicemode.h"
#include "downloadprogressdialog.h"
#include "keyboard.h"

// ── Version control panels ────────────────────────────────────────────────────
// Forward-declare to avoid pulling heavy headers into every translation unit
// that includes mainwindow.h.
namespace Kayte::Svn { class SvnPanel; }

// ─── TerminalWidget ───────────────────────────────────────────────────────────
// Embedded bash terminal: runs /bin/bash as a child process and pipes I/O
// to a QPlainTextEdit output view + QLineEdit command input.
class TerminalWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget() override;

    // Send a command directly to the running bash shell (e.g. cd into a project).
    void runCommand(const QString &command);

private slots:
    void onReadyReadStdOut();
    void onReadyReadStdErr();
    void onReturnPressed();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void setupUi();
    void appendOutput(const QString &text, bool isError = false);

    QPlainTextEdit *m_output  { nullptr };
    QLineEdit      *m_input   { nullptr };
    QProcess       *m_process { nullptr };
};
// ─────────────────────────────────────────────────────────────────────────────

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
    // ── File ──────────────────────────────────────────────────────────────────
    void on_actionNewFile_triggered();
    void handleOpenFileTriggered();
    void handleSaveFileTriggered();
    bool handleSaveFileAsTriggered();
    void on_actionCloseTab_triggered();
    void saveProjectAs();

    // ── Tab management ────────────────────────────────────────────────────────
    void on_tabWidgetEditor_tabCloseRequested(int index);
    void on_tabWidgetEditor_currentChanged(int index);
    void onTabClosed(QObject *obj = nullptr);

    // ── Tab title helpers ─────────────────────────────────────────────────────
    void updateTabTitle(bool modified);
    void updateTabTitleOnRename(const QString &newTitle);
    void handleTabModificationChanged(bool modified);
    void handleTabTitleChanged(const QString &title);

    // ── Build / Run ───────────────────────────────────────────────────────────
    void buildProject();
    void runProject();
    void cleanProject();
    void debugProject();

    // ── Tools menu (SVN + Git) ────────────────────────────────────────────────
    void setupToolsMenu();
    void setCurrentProjectPath(const QString &path);

    // ── Terminal dock ─────────────────────────────────────────────────────────
    void onToggleTerminal();

    // ── .xproj scaffold ───────────────────────────────────────────────────────
    void onCreateXProj();

    // ── Widget palette / UI designer ──────────────────────────────────────────
    void onToggleWidgetPalette();
    void onExportUiFile();
    void onClearCanvas();
    void onNewUiFile();

    // ── Project panel ─────────────────────────────────────────────────────────
    void onOpenProjectFolder();
    void onProjectTreeDoubleClicked(const QModelIndex &index);
    void onProjectTreeContextMenu(const QPoint &pos);
    void onCollapseAll();
    void onRefreshProject();

    // ── File browser ──────────────────────────────────────────────────────────
    void handlePathLineEditReturnPressed();
    void handleListViewDoubleClicked(const QModelIndex &index);

    // ── Dialogs ───────────────────────────────────────────────────────────────
    void showAboutDialog();
    void showModeSelectionDialog();
    void on_actionNewProject_triggered();

    // ── Mode / download ───────────────────────────────────────────────────────
    void activateMode(ChoiceMode::DevelopmentMode mode);
    void handleDownloadDialogFinished();

    // ── Line number area ──────────────────────────────────────────────────────
    void updateLineNumberAreaWidth(int newBlockCount);

private:
    // ── UI ────────────────────────────────────────────────────────────────────
    Ui::MainWindow *ui;

    // ── Editor tabs ───────────────────────────────────────────────────────────
    QVector<EditorTabWidget *> openEditorTabs;
    EditorTabWidget           *currentEditorTab() const;
    void                       createNewTab(const QString &filePath = QString());
    bool                       saveCurrentFile();

    // ── Project panel (left dock) ──────────────────────────────────────────────
    QDockWidget            *m_projectDock        { nullptr };
    QTreeView              *m_projectTree        { nullptr };
    QFileSystemModel       *m_projectModel       { nullptr };
    QSortFilterProxyModel  *m_projectProxy       { nullptr };
    QLabel                 *m_projectNameLabel   { nullptr };
    QAction                *m_actProjectPanel    { nullptr };
    void                    setupProjectPanel();
    void                    setProjectRoot(const QString &path);

    // ── File browser ──────────────────────────────────────────────────────────
    QTreeView        *fileListView    { nullptr };
    QFileSystemModel *fileSystemModel { nullptr };
    QLineEdit        *pathLineEdit    { nullptr };
    QPushButton      *browseButton    { nullptr };
    void              setupFileBrowser();
    void              setCurrentPath(const QString &path);

    // ── Version control panels ────────────────────────────────────────────────
    Kayte::Svn::SvnPanel  *m_svnPanel  { nullptr };
    Kayte::GitClientPanel *m_gitPanel  { nullptr };
    QDockWidget           *m_gitDock   { nullptr };
    QString                m_currentProjectPath;

    // ── Terminal dock ─────────────────────────────────────────────────────────
    TerminalWidget *m_terminalWidget { nullptr };
    QDockWidget    *m_terminalDock   { nullptr };
    QAction        *m_actTerminal    { nullptr };  // checkable – toggles dock

    // ── Font Awesome ──────────────────────────────────────────────────────────
    // Load fa-solid-900.ttf from Qt resources (:/fa-solid-900.ttf) once, then
    // use m_faFont to render any ICON_FA_* glyph string on a QLabel / QAction.
    QFont m_faFont;
    void  setupFontAwesome();
    void  setupTerminalDock();
    void  setupWidgetPalette();

    // ── Widget Palette Dock ───────────────────────────────────────────────────
    WidgetPaletteDock *m_paletteDock   { nullptr };
    QDockWidget       *m_designerDock  { nullptr };
    UiCanvasWidget    *m_canvas        { nullptr };
    QAction           *m_actPalette    { nullptr };
    QAction           *m_actDesigner   { nullptr };

    // ── Project persistence ───────────────────────────────────────────────────
    QString m_currentProjectFilePath;
    QString m_currentProjectName;

    // ── Build commands (defaults; overridden by loaded project settings) ──────
    QString m_buildCommand  { QStringLiteral("make all")                 };
    QString m_runCommand    { QStringLiteral("./output_executable")      };
    QString m_cleanCommand  { QStringLiteral("make clean")               };
    QString m_debugCommand  { QStringLiteral("lldb ./output_executable") };

    // ── Download / mode ───────────────────────────────────────────────────────
    QString                     defaultDownloadPath;
    QStringList                 radModeRepos;
    QStringList                 editorModeRepos;
    void                        setupDownloadRepos();
    ChoiceMode::DevelopmentMode currentDevelopmentMode { ChoiceMode::TextEditor };

    // ── Project list ──────────────────────────────────────────────────────────
    void populateProjectList();

    // ── Keyboard shortcuts ────────────────────────────────────────────────────
    KeyboardShortcutsManager *m_keyboardShortcutsManager { nullptr };
};

#endif // MAINWINDOW_H