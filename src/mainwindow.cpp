#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt Includes
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QFile>
#include <QIcon>
#include <QFileInfo>
#include <QPixmap>
#include <QTextDocument>
#include <QFont>
#include <QFontDatabase>
#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTreeView>
#include <QXmlStreamWriter>
#include <QProcess>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QInputDialog>
#include <QToolBar>
#include <QScrollArea>
#include <QSplitter>
#include "widgetpalettedock.h"
#include "uicanvaswidget.h"

// IconFontCppHeaders – maps Font Awesome glyph names to Unicode code points.
// Provided via CMake FetchContent (IconFontCppHeaders). If not yet in your
// build, comment out the include and the ICON_FA_* usages below.
#if __has_include(<IconsFontAwesome6.h>)
#  include <IconsFontAwesome6.h>
#  define KAYTEIDE_FA_AVAILABLE 1
#else
// Fallback plain-text labels so the project compiles without the header.
#  define ICON_FA_TERMINAL   "\xef\x84\xa0"  // U+F120
#  define ICON_FA_FOLDER_PLUS "\xef\x99\x9e"  // U+F65E
#  define KAYTEIDE_FA_AVAILABLE 0
#endif

// Custom widgets and dialogs
#include "editortabwidget.h"
#include "choicemode.h"
#include "downloadprogressdialog.h"
#include "keyboard.h"

// ── Version control panels ────────────────────────────────────────────────────
// These live in src/svn/ and gitclient/include/ respectively.
// Comment out either include if the module is not yet in your build.
#include "svn/SvnPanel.h"
#include "GitClientPanel.hpp"   // from gitclient/include/
using Kayte::GitClientPanel;
// ─────────────────────────────────────────────────────────────────────────────
// TerminalWidget implementation
// Spawns /bin/bash and wires its stdin/stdout/stderr to a dark QPlainTextEdit.
// ─────────────────────────────────────────────────────────────────────────────

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &TerminalWidget::onReadyReadStdOut);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &TerminalWidget::onReadyReadStdErr);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalWidget::onProcessFinished);

    m_process->start(QStringLiteral("/bin/bash"),
                     QStringList() << QStringLiteral("--norc") << QStringLiteral("-i"));

    if (!m_process->waitForStarted(3000))
        appendOutput(QStringLiteral("[ERROR] Could not start /bin/bash\n"), true);
    else
        appendOutput(QStringLiteral("[bash] ready\n"));
}

TerminalWidget::~TerminalWidget()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write("exit\n");
        m_process->waitForFinished(1500);
        m_process->kill();
    }
}

void TerminalWidget::setupUi()
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->setSpacing(2);

    // ── Output view (dark background, monospace) ──────────────────────────────
    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont mono(QStringLiteral("Monospace"), 10);
    mono.setStyleHint(QFont::TypeWriter);
    m_output->setFont(mono);

    QPalette pal = m_output->palette();
    pal.setColor(QPalette::Base, QColor(0x1e, 0x1e, 0x1e));
    pal.setColor(QPalette::Text, QColor(0xd4, 0xd4, 0xd4));
    m_output->setPalette(pal);
    m_output->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ── Input row ─────────────────────────────────────────────────────────────
    auto *inputRow = new QHBoxLayout;
    auto *prompt   = new QLabel(QStringLiteral("$ "), this);
    prompt->setFont(mono);

    m_input = new QLineEdit(this);
    m_input->setFont(mono);
    m_input->setPlaceholderText(QStringLiteral("Enter command…"));

    auto *runBtn = new QPushButton(QStringLiteral("Run"), this);
    runBtn->setFixedWidth(48);
    connect(runBtn,  &QPushButton::clicked,   this, &TerminalWidget::onReturnPressed);
    connect(m_input, &QLineEdit::returnPressed, this, &TerminalWidget::onReturnPressed);

    inputRow->addWidget(prompt);
    inputRow->addWidget(m_input, 1);
    inputRow->addWidget(runBtn);

    lay->addWidget(m_output, 1);
    lay->addLayout(inputRow);
}

void TerminalWidget::runCommand(const QString &command)
{
    if (!m_process || m_process->state() != QProcess::Running) return;
    appendOutput(QStringLiteral("$ ") + command + QLatin1Char('\n'));
    m_process->write((command + QLatin1Char('\n')).toUtf8());
}

void TerminalWidget::onReturnPressed()
{
    const QString cmd = m_input->text().trimmed();
    if (cmd.isEmpty()) return;
    m_input->clear();
    runCommand(cmd);
}

void TerminalWidget::onReadyReadStdOut()
{
    appendOutput(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
}

void TerminalWidget::onReadyReadStdErr()
{
    appendOutput(QString::fromLocal8Bit(m_process->readAllStandardError()), true);
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    appendOutput(QStringLiteral("\n[bash exited with code %1]\n").arg(exitCode), true);
}

void TerminalWidget::appendOutput(const QString &text, bool isError)
{
    QTextCharFormat fmt;
    fmt.setForeground(isError ? QColor(0xff, 0x66, 0x66) : QColor(0xd4, 0xd4, 0xd4));
    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, fmt);
    m_output->setTextCursor(cursor);
    m_output->verticalScrollBar()->setValue(m_output->verticalScrollBar()->maximum());
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_keyboardShortcutsManager(new KeyboardShortcutsManager(this))
    , m_svnPanel(nullptr)
    , m_gitPanel(nullptr)
    , m_gitDock(nullptr)
    , m_terminalWidget(nullptr)
    , m_terminalDock(nullptr)
    , m_actTerminal(nullptr)
{
    ui->setupUi(this);
    setWindowTitle(tr("Kayte IDE"));

    qDebug() << "DEBUG: MainWindow constructor - ui pointer:" << ui;
    if (ui)
        qDebug() << "DEBUG: MainWindow constructor - ui->tabWidgetEditor pointer:" << ui->tabWidgetEditor;
    else
        qDebug() << "DEBUG: MainWindow constructor - ui is nullptr!";

    // --- Central Widget ---
    setCentralWidget(ui->tabWidgetEditor);
    ui->tabWidgetEditor->setTabsClosable(true);
    ui->tabWidgetEditor->setMovable(true);

    connect(ui->tabWidgetEditor, &QTabWidget::tabCloseRequested,
            this, &MainWindow::on_tabWidgetEditor_tabCloseRequested);

    createNewTab();

    // --- File paths ---
    defaultDownloadPath = QDir::homePath() + QDir::separator() + "KayteIDE_Resources";
    QDir().mkpath(defaultDownloadPath);

    setupDownloadRepos();

    // --- Menu & Toolbar Icons ---
    ui->actionNewFile->setIcon(QIcon::fromTheme("document-new"));
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open"));
    ui->actionSave->setIcon(QIcon::fromTheme("document-save"));
    ui->actionSave_As->setIcon(QIcon::fromTheme("document-save-as"));
    ui->actionCloseTab->setIcon(QIcon::fromTheme("tab-close"));
    ui->actionExit->setIcon(QIcon::fromTheme("application-exit"));
    ui->actionBuild->setIcon(QIcon::fromTheme("system-run"));
    ui->actionClean->setIcon(QIcon::fromTheme("edit-clear"));
    ui->actionRun->setIcon(QIcon::fromTheme("media-playback-start"));
    ui->actionDebug->setIcon(QIcon::fromTheme("tools-debugger"));
    ui->actionAbout->setIcon(QIcon::fromTheme("help-about"));

    // --- File / Edit / Build connections ---
    connect(ui->actionNewFile,      &QAction::triggered, this, &MainWindow::on_actionNewFile_triggered);
    connect(ui->actionOpen,         &QAction::triggered, this, &MainWindow::handleOpenFileTriggered);
    connect(ui->actionSave,         &QAction::triggered, this, &MainWindow::handleSaveFileTriggered);
    connect(ui->actionSave_As,      &QAction::triggered, this, &MainWindow::handleSaveFileAsTriggered);
    connect(ui->actionCloseTab,     &QAction::triggered, this, &MainWindow::on_actionCloseTab_triggered);
    connect(ui->actionExit,         &QAction::triggered, this, &QWidget::close);
    connect(ui->actionBuild,        &QAction::triggered, this, &MainWindow::buildProject);
    connect(ui->actionRun,          &QAction::triggered, this, &MainWindow::runProject);
    connect(ui->actionClean,        &QAction::triggered, this, &MainWindow::cleanProject);
    connect(ui->actionDebug,        &QAction::triggered, this, &MainWindow::debugProject);
    connect(ui->actionAbout,        &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(ui->actionSaveProjectAs,&QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(ui->actionNewProject,   &QAction::triggered, this, &MainWindow::on_actionNewProject_triggered);

    connect(ui->actionCut,       &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerCut);
    connect(ui->actionCopy,      &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerCopy);
    connect(ui->actionPaste,     &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerPaste);
    connect(ui->actionSelectAll, &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerSelectAll);

    connect(ui->tabWidgetEditor, &QTabWidget::currentChanged,
            this, &MainWindow::on_tabWidgetEditor_currentChanged);

    // --- Tools menu (SVN + Git panels) ---
    setupToolsMenu();

    // --- Font Awesome + Terminal dock ---
    setupFontAwesome();
    setupTerminalDock();

    // --- Widget Palette + UI Designer ---
    setupWidgetPalette();

    // --- Mode selection (queued so the window shows first) ---
    QMetaObject::invokeMethod(this, "showModeSelectionDialog", Qt::QueuedConnection);
}

// ─────────────────────────────────────────────────────────────────────────────
// Widget Palette Dock (left) + UI Designer Canvas (central area tab)
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupWidgetPalette()
{
    // ── Left palette dock ─────────────────────────────────────────────────────
    m_paletteDock = new WidgetPaletteDock(m_faFont, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_paletteDock);
    m_paletteDock->hide(); // shown on demand

    // ── Designer canvas wrapped in a scrollable QDockWidget ───────────────────
    m_canvas = new UiCanvasWidget;

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidget(m_canvas);
    scrollArea->setWidgetResizable(false);
    scrollArea->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { border: none; background: #e8e9ec; }"));

    m_designerDock = new QDockWidget(tr("UI Designer"), this);
    m_designerDock->setObjectName(QStringLiteral("UiDesignerDock"));
    m_designerDock->setWidget(scrollArea);
    m_designerDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_designerDock->setFeatures(QDockWidget::DockWidgetMovable |
                                 QDockWidget::DockWidgetFloatable |
                                 QDockWidget::DockWidgetClosable);

    // Custom title bar with FA icon + action buttons
    auto *dtb    = new QWidget(m_designerDock);
    auto *dtbLay = new QHBoxLayout(dtb);
    dtbLay->setContentsMargins(6, 2, 4, 2);
    dtbLay->setSpacing(4);

    auto *dIcon = new QLabel(QString::fromUtf8(ICON_FA_IMAGE), dtb);
    dIcon->setFont(m_faFont);
    auto *dTitle = new QLabel(tr("  UI Designer"), dtb);
    QFont dbf = dTitle->font(); dbf.setBold(true); dTitle->setFont(dbf);

    auto *btnExport = new QPushButton(tr("Export .ui"), dtb);
    btnExport->setFixedHeight(22);
    btnExport->setToolTip(tr("Save the canvas as a Qt .ui file"));

    auto *btnClear = new QPushButton(tr("Clear"), dtb);
    btnClear->setFixedHeight(22);
    btnClear->setToolTip(tr("Remove all widgets from canvas"));

    auto *btnNew = new QPushButton(tr("New"), dtb);
    btnNew->setFixedHeight(22);
    btnNew->setToolTip(tr("Start a new empty form"));

    dtbLay->addWidget(dIcon);
    dtbLay->addWidget(dTitle);
    dtbLay->addStretch();
    dtbLay->addWidget(btnNew);
    dtbLay->addWidget(btnClear);
    dtbLay->addWidget(btnExport);
    m_designerDock->setTitleBarWidget(dtb);

    connect(btnExport, &QPushButton::clicked, this, &MainWindow::onExportUiFile);
    connect(btnClear,  &QPushButton::clicked, this, &MainWindow::onClearCanvas);
    connect(btnNew,    &QPushButton::clicked, this, &MainWindow::onNewUiFile);

    // Status line: selected widget info
    connect(m_canvas, &UiCanvasWidget::itemSelectionChanged, this,
        [this](const CanvasItem *item) {
            if (item)
                statusBar()->showMessage(
                    tr("Selected: %1  [%2]  @ (%3, %4)  %5 × %6")
                    .arg(item->objectName, item->widgetType)
                    .arg(item->geometry.x()).arg(item->geometry.y())
                    .arg(item->geometry.width()).arg(item->geometry.height()));
            else
                statusBar()->showMessage(tr("Ready"));
        });

    addDockWidget(Qt::RightDockWidgetArea, m_designerDock);
    m_designerDock->hide();

    // ── Toolbar buttons ───────────────────────────────────────────────────────
    QToolBar *tb = addToolBar(tr("UI Designer"));
    tb->setObjectName(QStringLiteral("UiDesignerToolBar"));

#if __has_include(<IconsFontAwesome6.h>)
    m_actPalette = new QAction(QString::fromUtf8(ICON_FA_OBJECT_GROUP), this);
    m_actPalette->setFont(m_faFont);
    m_actDesigner = new QAction(QString::fromUtf8(ICON_FA_IMAGE), this);
    m_actDesigner->setFont(m_faFont);
#else
    m_actPalette  = new QAction(tr("[Palette]"), this);
    m_actDesigner = new QAction(tr("[Designer]"), this);
#endif

    m_actPalette->setToolTip(tr("Toggle Widget Palette"));
    m_actPalette->setStatusTip(tr("Show / hide the Qt widget palette"));
    m_actPalette->setCheckable(true);
    m_actPalette->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(m_actPalette, &QAction::triggered, this, &MainWindow::onToggleWidgetPalette);
    tb->addAction(m_actPalette);

    m_actDesigner->setToolTip(tr("Toggle UI Designer canvas"));
    m_actDesigner->setStatusTip(tr("Show / hide the .ui form designer canvas"));
    m_actDesigner->setCheckable(true);
    m_actDesigner->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    connect(m_actDesigner, &QAction::triggered, this, &MainWindow::onToggleWidgetPalette);
    tb->addAction(m_actDesigner);

    // Mirror in View menu
    QMenu *viewMenu = nullptr;
    for (QAction *a : menuBar()->actions())
        if (a->menu() && a->text().contains(tr("View"), Qt::CaseInsensitive))
            { viewMenu = a->menu(); break; }
    if (!viewMenu) {
        viewMenu = new QMenu(tr("&View"), this);
        menuBar()->insertMenu(menuBar()->actions().isEmpty()
                              ? nullptr : menuBar()->actions().last(), viewMenu);
    }
    viewMenu->addSeparator();
    viewMenu->addAction(m_actPalette);
    viewMenu->addAction(m_actDesigner);

    // Keep actions in sync with dock visibility
    connect(m_paletteDock,  &QDockWidget::visibilityChanged, m_actPalette,  &QAction::setChecked);
    connect(m_designerDock, &QDockWidget::visibilityChanged, m_actDesigner, &QAction::setChecked);
}

void MainWindow::onToggleWidgetPalette()
{
    const bool showPalette  = m_actPalette  ? m_actPalette->isChecked()  : !m_paletteDock->isVisible();
    const bool showDesigner = m_actDesigner ? m_actDesigner->isChecked() : !m_designerDock->isVisible();
    m_paletteDock->setVisible(showPalette);
    m_designerDock->setVisible(showDesigner);
}

void MainWindow::onExportUiFile()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export .ui File"),
        m_currentProjectFilePath.isEmpty()
            ? QDir::homePath() + QStringLiteral("/form.ui")
            : QFileInfo(m_currentProjectFilePath).absolutePath() + QStringLiteral("/form.ui"),
        tr("Qt UI Files (*.ui);;All Files (*.*)"));
    if (path.isEmpty()) return;

    if (m_canvas->exportUiFile(path)) {
        statusBar()->showMessage(tr("Exported: %1").arg(path), 4000);
        if (m_terminalWidget) {
            const QString cmd = QLatin1String("echo 'UI file exported:' && ls -lh \"")
                                + path + QLatin1Char('"');
            m_terminalWidget->runCommand(cmd);
        }
    } else {
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Could not write file:\n%1").arg(path));
    }
}

void MainWindow::onClearCanvas()
{
    if (!m_canvas || m_canvas->itemCount() == 0) return;
    if (QMessageBox::question(this, tr("Clear Canvas"),
            tr("Remove all widgets from the canvas?"),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        m_canvas->clearCanvas();
}

void MainWindow::onNewUiFile()
{
    if (m_canvas && m_canvas->itemCount() > 0) {
        const auto btn = QMessageBox::question(this, tr("New Form"),
            tr("Discard the current form and start a new one?"),
            QMessageBox::Yes | QMessageBox::No);
        if (btn != QMessageBox::Yes) return;
    }
    if (m_canvas) m_canvas->clearCanvas();
    // Show both docks so the user can start designing immediately
    if (m_paletteDock)  { m_paletteDock->show();  m_paletteDock->raise(); }
    if (m_designerDock) { m_designerDock->show(); m_designerDock->raise(); }
    statusBar()->showMessage(tr("New form – drag widgets from the palette onto the canvas"), 5000);
}

// ─────────────────────────────────────────────────────────────────────────────
// Font Awesome
// ─────────────────────────────────────────────────────────────────────────────
// Loads fa-solid-900.ttf from the Qt resource system (:/fa-solid-900.ttf).
// Add to resources.qrc:
//   <file alias="fa-solid-900.ttf">fonts/fa-solid-900.ttf</file>
// Download the TTF from:
//   https://use.fontawesome.com/releases/v6.5.1/fontawesome-free-6.5.1-desktop.zip
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupFontAwesome()
{
    const int id = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fa-solid-900.ttf"));

    if (id < 0) {
        qWarning() << "[FA] fa-solid-900.ttf not found in Qt resources — "
                      "icon glyphs will fall back to placeholder text. "
                      "Add the font to resources.qrc to enable icons.";
        m_faFont = QFont(QStringLiteral("Monospace"), 14);
    } else {
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        m_faFont = QFont(families.first(), 14);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Terminal Dock
// Creates the QDockWidget that holds the TerminalWidget and adds a toolbar
// button (Font Awesome terminal icon) + a matching View menu action.
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupTerminalDock()
{
    // ── Create the dock ────────────────────────────────────────────────────────
    m_terminalDock = new QDockWidget(tr("Terminal"), this);
    m_terminalDock->setObjectName(QStringLiteral("TerminalDock"));
    m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea |
                                     Qt::TopDockWidgetArea);
    m_terminalDock->setFeatures(QDockWidget::DockWidgetMovable   |
                                 QDockWidget::DockWidgetFloatable  |
                                 QDockWidget::DockWidgetClosable);

    // ── Custom title bar with FA terminal icon ─────────────────────────────────
    auto *titleBar    = new QWidget(m_terminalDock);
    auto *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(6, 2, 6, 2);

    auto *iconLbl = new QLabel(QString::fromUtf8(ICON_FA_TERMINAL), titleBar);
    iconLbl->setFont(m_faFont);

    auto *textLbl = new QLabel(tr("  Terminal"), titleBar);
    QFont bold    = textLbl->font();
    bold.setBold(true);
    textLbl->setFont(bold);

    titleLayout->addWidget(iconLbl);
    titleLayout->addWidget(textLbl);
    titleLayout->addStretch();
    m_terminalDock->setTitleBarWidget(titleBar);

    // ── Embed the TerminalWidget ───────────────────────────────────────────────
    m_terminalWidget = new TerminalWidget(m_terminalDock);
    m_terminalDock->setWidget(m_terminalWidget);
    m_terminalDock->setMinimumHeight(160);

    // Stack in the same bottom area as SVN / Git docks
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);
    if (m_gitDock)
        tabifyDockWidget(m_gitDock, m_terminalDock);

    // ── Toolbar: Font Awesome "terminal" glyph + "folder-plus" (.xproj) ───────
    QToolBar *tb = addToolBar(tr("Terminal / Project"));
    tb->setObjectName(QStringLiteral("TerminalToolBar"));

    // Terminal toggle button
    m_actTerminal = new QAction(QString::fromUtf8(ICON_FA_TERMINAL), this);
    m_actTerminal->setFont(m_faFont);
    m_actTerminal->setToolTip(tr("Toggle Terminal panel  (Ctrl+`)"));
    m_actTerminal->setStatusTip(tr("Show / hide the integrated bash terminal"));
    m_actTerminal->setCheckable(true);
    m_actTerminal->setChecked(true);
    m_actTerminal->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
    connect(m_actTerminal, &QAction::triggered, this, &MainWindow::onToggleTerminal);
    tb->addAction(m_actTerminal);

    // .xproj scaffold button
    auto *actXProj = new QAction(QString::fromUtf8(ICON_FA_FOLDER_PLUS), this);
    actXProj->setFont(m_faFont);
    actXProj->setToolTip(tr("New .xproj project scaffold"));
    actXProj->setStatusTip(tr("Create a new .xproj folder structure"));
    actXProj->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    connect(actXProj, &QAction::triggered, this, &MainWindow::onCreateXProj);
    tb->addAction(actXProj);

    // ── Keep dock visibility in sync with the toolbar toggle button ───────────
    connect(m_terminalDock, &QDockWidget::visibilityChanged,
            m_actTerminal, &QAction::setChecked);

    // ── Mirror the action in the View menu (create it if absent) ─────────────
    QMenu *viewMenu = nullptr;
    for (QAction *a : menuBar()->actions()) {
        if (a->menu() && a->text().contains(tr("View"), Qt::CaseInsensitive)) {
            viewMenu = a->menu();
            break;
        }
    }
    if (!viewMenu) {
        viewMenu = new QMenu(tr("&View"), this);
        QAction *before = menuBar()->actions().isEmpty()
                          ? nullptr : menuBar()->actions().last();
        menuBar()->insertMenu(before, viewMenu);
    }
    viewMenu->addAction(m_actTerminal);

    auto *actXProjMenu = viewMenu->addAction(tr("New .&xproj…"));
    actXProjMenu->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    connect(actXProjMenu, &QAction::triggered, this, &MainWindow::onCreateXProj);
}

// ─────────────────────────────────────────────────────────────────────────────
// Toggle Terminal
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onToggleTerminal()
{
    m_terminalDock->setVisible(!m_terminalDock->isVisible());
}

// ─────────────────────────────────────────────────────────────────────────────
// Create .xproj scaffold
//
// Prompted layout:
//   <ProjectName>/
//   ├── src/
//   │   └── main.kayte
//   ├── include/
//   ├── assets/
//   ├── build/
//   └── <ProjectName>.xproj
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onCreateXProj()
{
    // 1. Project name
    bool ok = false;
    const QString projectName = QInputDialog::getText(
        this, tr("New .xproj Project"),
        tr("Project name:"), QLineEdit::Normal,
        QStringLiteral("MyKayteProject"), &ok).trimmed();
    if (!ok || projectName.isEmpty()) return;

    // 2. Parent directory
    const QString parentDir = QFileDialog::getExistingDirectory(
        this, tr("Select parent directory"), QDir::homePath());
    if (parentDir.isEmpty()) return;

    QDir root(parentDir);

    // 3. Create directories
    const QStringList subdirs = {
        projectName,
        projectName + QStringLiteral("/src"),
        projectName + QStringLiteral("/include"),
        projectName + QStringLiteral("/assets"),
        projectName + QStringLiteral("/build"),
    };
    for (const QString &sub : subdirs) {
        if (!root.mkpath(sub)) {
            QMessageBox::critical(this, tr("Error"),
                tr("Could not create directory: %1/%2").arg(parentDir, sub));
            return;
        }
    }

    // 4. Write <ProjectName>.xproj manifest (XML)
    const QString projRoot = parentDir + QLatin1Char('/') + projectName;
    const QString xprojPath = projRoot + QLatin1Char('/') + projectName + QStringLiteral(".xproj");

    QFile xproj(xprojPath);
    if (xproj.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QXmlStreamWriter xml(&xproj);
        xml.setAutoFormatting(true);
        xml.setAutoFormattingIndent(4);
        xml.writeStartDocument();
        xml.writeStartElement(QStringLiteral("XProject"));
        xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
        xml.writeTextElement(QStringLiteral("Name"),       projectName);
        xml.writeTextElement(QStringLiteral("Language"),   QStringLiteral("Kayte"));
        xml.writeTextElement(QStringLiteral("SourceDir"),  QStringLiteral("src"));
        xml.writeTextElement(QStringLiteral("IncludeDir"), QStringLiteral("include"));
        xml.writeTextElement(QStringLiteral("AssetsDir"),  QStringLiteral("assets"));
        xml.writeTextElement(QStringLiteral("BuildDir"),   QStringLiteral("build"));
        xml.writeStartElement(QStringLiteral("Files"));
        xml.writeTextElement(QStringLiteral("File"), QStringLiteral("src/main.kayte"));
        xml.writeEndElement(); // Files
        xml.writeEndElement(); // XProject
        xml.writeEndDocument();
        xproj.close();
    }

    // 5. Write a starter main.kayte
    QFile mainKayte(projRoot + QStringLiteral("/src/main.kayte"));
    if (mainKayte.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&mainKayte);
        ts << QStringLiteral("// ") << projectName
           << QStringLiteral(" – created with KayteIDE\n\n")
           << QStringLiteral("program ") << projectName << QStringLiteral(";\n\n")
           << QStringLiteral("begin\n")
           << QStringLiteral("  writeln('Hello from ") << projectName
           << QStringLiteral("!');\n")
           << QStringLiteral("end.\n");
        mainKayte.close();
    }

    // 6. Auto-set the project working directory and show the tree in the terminal
    setCurrentProjectPath(projRoot);

    if (m_terminalDock) {
        m_terminalDock->show();
        m_terminalDock->raise();
    }
    if (m_terminalWidget)
        m_terminalWidget->runCommand(
            QStringLiteral("echo '=== Project created ===' && ls -R \"") +
            projRoot + QStringLiteral("\""));

    statusBar()->showMessage(
        tr("Project \"%1\" created at %2").arg(projectName, parentDir), 4000);

    QMessageBox::information(this, tr("Project Created"),
        tr("Project <b>%1</b> was scaffolded at:<br><code>%2</code>")
            .arg(projectName, projRoot));
}

// ─────────────────────────────────────────────────────────────────────────────
// Tools menu: Subversion and Git dock panels
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupToolsMenu()
{
    // Reuse an existing "Tools" menu if the .ui file already defines one,
    // otherwise create it and insert it before "Help".
    QMenu *toolsMenu = nullptr;
    for (QAction *a : menuBar()->actions()) {
        if (a->menu() && a->text().contains(tr("Tools"), Qt::CaseInsensitive)) {
            toolsMenu = a->menu();
            break;
        }
    }
    if (!toolsMenu) {
        toolsMenu = new QMenu(tr("&Tools"), this);
        // Insert before the last menu (typically "Help")
        QAction *before = menuBar()->actions().isEmpty() ? nullptr
                                                         : menuBar()->actions().last();
        menuBar()->insertMenu(before, toolsMenu);
    }

    // ── SVN Panel ────────────────────────────────────────────────────────────
    m_svnPanel = new Kayte::Svn::SvnPanel(this);
    m_svnPanel->setObjectName("SvnDockPanel");
    addDockWidget(Qt::BottomDockWidgetArea, m_svnPanel);
    m_svnPanel->hide(); // hidden until user requests it

    QAction *svnAction = toolsMenu->addAction(
        QIcon::fromTheme("svn"),
        tr("&Subversion (SVN)…"));
    svnAction->setCheckable(true);
    svnAction->setChecked(false);
    svnAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    svnAction->setStatusTip(tr("Show / hide the Subversion panel"));

    connect(svnAction, &QAction::toggled, this, [this](bool checked) {
        m_svnPanel->setVisible(checked);
        if (checked && !m_currentProjectPath.isEmpty())
            m_svnPanel->setWorkingCopy(m_currentProjectPath);
    });
    connect(m_svnPanel, &QDockWidget::visibilityChanged,
            svnAction, &QAction::setChecked);

    // Open file from SVN panel in editor
    connect(m_svnPanel, &Kayte::Svn::SvnPanel::openFileRequested,
            this, &MainWindow::createNewTab);

    // ── Git Panel ─────────────────────────────────────────────────────────────
    // GitClientPanel is NOT a QDockWidget, so we wrap it in one.
    m_gitPanel = new GitClientPanel(this);
    m_gitDock  = new QDockWidget(tr("Git"), this);
    m_gitDock->setObjectName("GitDockPanel");
    m_gitDock->setWidget(m_gitPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_gitDock);
    tabifyDockWidget(m_svnPanel, m_gitDock); // stack SVN and Git in the same area
    m_gitDock->hide();

    QAction *gitAction = toolsMenu->addAction(
        QIcon::fromTheme("git"),
        tr("&Git…"));
    gitAction->setCheckable(true);
    gitAction->setChecked(false);
    gitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    gitAction->setStatusTip(tr("Show / hide the Git panel"));

    connect(gitAction, &QAction::toggled, this, [this](bool checked) {
        m_gitDock->setVisible(checked);
        if (checked && !m_currentProjectPath.isEmpty())
            m_gitPanel->openRepository(m_currentProjectPath);
    });
    connect(m_gitDock, &QDockWidget::visibilityChanged,
            gitAction, &QAction::setChecked);

    toolsMenu->addSeparator();

    // ── Version control → Set working directory ───────────────────────────────
    QAction *setVcDir = toolsMenu->addAction(
        QIcon::fromTheme("folder"),
        tr("Set &Working Directory…"));
    setVcDir->setStatusTip(tr("Set the root directory used by the SVN and Git panels"));
    connect(setVcDir, &QAction::triggered, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Set Working Directory"), m_currentProjectPath);
        if (dir.isEmpty()) return;
        m_currentProjectPath = dir;
        m_svnPanel->setWorkingCopy(dir);
        m_gitPanel->openRepository(dir);
        statusBar()->showMessage(tr("Working directory: %1").arg(dir), 4000);
    });

    toolsMenu->addSeparator();

    // ── Keyboard shortcuts reference ─────────────────────────────────────────
    QAction *kbAction = toolsMenu->addAction(
        QIcon::fromTheme("preferences-desktop-keyboard"),
        tr("&Keyboard Shortcuts…"));
    kbAction->setStatusTip(tr("Show keyboard shortcuts reference"));
    connect(kbAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, tr("Keyboard Shortcuts"),
            tr("<b>File</b><br>"
               "Ctrl+N – New file<br>"
               "Ctrl+O – Open file<br>"
               "Ctrl+S – Save<br>"
               "Ctrl+Shift+S – SVN panel<br>"
               "Ctrl+Shift+G – Git panel<br>"
               "<br><b>Build</b><br>"
               "F5 – Run<br>"
               "F6 – Build<br>"
               "F7 – Clean<br>"
               "F8 – Debug"));
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Project open/change – auto-update VC panels
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setCurrentProjectPath(const QString &path)
{
    m_currentProjectPath = path;
    if (m_svnPanel && m_svnPanel->isVisible())
        m_svnPanel->setWorkingCopy(path);
    if (m_gitPanel && m_gitDock->isVisible())
        m_gitPanel->openRepository(path);
}

// ─────────────────────────────────────────────────────────────────────────────
// The rest of the original implementation (unchanged unless noted)
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_actionNewProject_triggered()
{
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // TODO: get project options and create project files
    }
}

void MainWindow::handleOpenFileTriggered()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open File"), QString(),
        tr("All Files (*);;Text Files (*.txt);;Source Files (*.cpp *.h *.cxx *.hpp)"
           ";;Visual Basic (*.vb);;Kayte Files (*.kayte *.kyt)"
           ";;Pascal Files (*.pas *.pp *.dpr);;Delphi Forms (*.dfm)"));
    if (filePath.isEmpty()) return;

    for (int i = 0; i < ui->tabWidgetEditor->count(); ++i) {
        EditorTabWidget *existing = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(i));
        if (existing && existing->filePath() == filePath) {
            ui->tabWidgetEditor->setCurrentIndex(i);
            return;
        }
    }

    EditorTabWidget *newTab = new EditorTabWidget(filePath, ui->tabWidgetEditor);
    if (newTab->loadFile(filePath)) {
        int idx = ui->tabWidgetEditor->addTab(newTab, QFileInfo(filePath).fileName());
        ui->tabWidgetEditor->setCurrentIndex(idx);
        connect(newTab, &EditorTabWidget::modificationChanged, this, &MainWindow::updateTabTitle);
        connect(newTab, &EditorTabWidget::titleChanged, this, &MainWindow::updateTabTitleOnRename);
        connect(newTab, &EditorTabWidget::destroyed, this, &MainWindow::onTabClosed);
    } else {
        newTab->deleteLater();
    }
}

void MainWindow::on_tabWidgetEditor_currentChanged(int index)
{
    EditorTabWidget *current = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(index));
    m_keyboardShortcutsManager->setTargetEditor(current ? current->getPlainTextEdit() : nullptr);
}

void MainWindow::setupFileBrowser()
{
    fileSystemModel = new QFileSystemModel(this);
    fileSystemModel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    fileSystemModel->setRootPath(QDir::homePath());

    QTreeView *fileTreeView = new QTreeView(this);
    fileTreeView->setModel(fileSystemModel);
    fileTreeView->hideColumn(1);
    fileTreeView->hideColumn(2);
    fileTreeView->hideColumn(3);

    pathLineEdit = new QLineEdit(this);
    connect(pathLineEdit, &QLineEdit::returnPressed,
            this, &MainWindow::handlePathLineEditReturnPressed);

    browseButton = new QPushButton("Browse...", this);
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this, "Open Directory", pathLineEdit->text(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) setCurrentPath(dir);
    });

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("Path:", this));
    pathLayout->addWidget(pathLineEdit);
    pathLayout->addWidget(browseButton);
    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(fileTreeView);
    setCentralWidget(centralWidget);

    fileListView = fileTreeView;
}

void MainWindow::setCurrentPath(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        pathLineEdit->setText(QDir::toNativeSeparators(path));
        fileListView->setRootIndex(fileSystemModel->setRootPath(path));
    } else {
        qWarning() << "Path does not exist:" << path;
    }
}

void MainWindow::handlePathLineEditReturnPressed()
{
    const QString newPath = pathLineEdit->text();
    if (QDir(newPath).exists()) {
        setCurrentPath(newPath);
    } else {
        QMessageBox::warning(this, tr("Path Not Found"),
                             tr("The path '%1' does not exist.").arg(newPath));
        pathLineEdit->setText(fileSystemModel->rootPath());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

EditorTabWidget* MainWindow::currentEditorTab() const
{
    return qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->currentWidget());
}

void MainWindow::onTabClosed(QObject *obj)
{
    EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(obj);
    if (tab)
        qDebug() << "EditorTabWidget destroyed for file:" << tab->filePath();
}

void MainWindow::createNewTab(const QString &filePath)
{
    if (filePath.isEmpty() && ui->tabWidgetEditor->count() > 0) {
        EditorTabWidget *current = currentEditorTab();
        if (current && current->filePath().isEmpty() && current->isModified()) {
            if (!saveCurrentFile()) return;
        }
    }

    EditorTabWidget *editorTab = new EditorTabWidget(filePath, ui->tabWidgetEditor);
    openEditorTabs.append(editorTab);

    connect(editorTab, &EditorTabWidget::modificationChanged,
            this, &MainWindow::updateTabTitle);
    connect(editorTab, &EditorTabWidget::titleChanged,
            this, &MainWindow::updateTabTitleOnRename);

    QString tabTitle = tr("Untitled");

    if (!filePath.isEmpty()) {
        if (editorTab->loadFile(filePath)) {
            tabTitle = QFileInfo(filePath).fileName();
        } else {
            openEditorTabs.removeOne(editorTab);
            editorTab->deleteLater();
            QMessageBox::warning(this, tr("File Open Error"),
                                 tr("Could not open file: %1").arg(filePath));
            return;
        }
    }

    int index = ui->tabWidgetEditor->addTab(editorTab, tabTitle);
    ui->tabWidgetEditor->setCurrentIndex(index);

    if (filePath.isEmpty()) {
        editorTab->setModified(true);
        updateTabTitle(true);
    }
    m_keyboardShortcutsManager->setTargetEditor(editorTab->getPlainTextEdit());
}

bool MainWindow::saveCurrentFile()
{
    EditorTabWidget *editorTab = currentEditorTab();
    if (!editorTab) return true;
    if (!editorTab->isModified()) return true;

    QString fileName = QFileInfo(editorTab->filePath()).fileName();
    if (fileName.isEmpty()) fileName = tr("Untitled");

    qDebug() << "saveCurrentFile: Showing dialog for" << fileName;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Unsaved Changes"));
    msgBox.setText(tr("The document '%1' has been modified.").arg(fileName));
    msgBox.setInformativeText(tr("Do you want to save your changes?"));
    msgBox.setIcon(QMessageBox::Warning);

    QPushButton *saveBtn    = msgBox.addButton(tr("Save"),       QMessageBox::AcceptRole);
    QPushButton *discardBtn = msgBox.addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
    QPushButton *cancelBtn  = msgBox.addButton(tr("Cancel"),     QMessageBox::RejectRole);
    msgBox.setDefaultButton(saveBtn);
    msgBox.setEscapeButton(cancelBtn);
    msgBox.exec();

    QAbstractButton *clicked = msgBox.clickedButton();
    qDebug() << "User clicked button";

    if (clicked == saveBtn) {
        qDebug() << "User chose to SAVE";
        bool ok = editorTab->filePath().isEmpty()
                ? handleSaveFileAsTriggered()
                : editorTab->saveFile(editorTab->filePath());
        qDebug() << "Save operation result:" << ok;
        return ok;
    } else if (clicked == discardBtn) {
        qDebug() << "User chose to DISCARD";
        return true;
    } else {
        qDebug() << "User chose to CANCEL";
        return false;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "=== closeEvent STARTED ===";
    qDebug() << "Checking" << ui->tabWidgetEditor->count() << "tabs for unsaved changes";

    for (int i = 0; i < ui->tabWidgetEditor->count(); ++i) {
        EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(i));
        if (tab && tab->isModified()) {
            qDebug() << "Tab" << i << "(" << QFileInfo(tab->filePath()).fileName() << ") is modified";
            ui->tabWidgetEditor->setCurrentIndex(i);
            if (!saveCurrentFile()) {
                qDebug() << "=== closeEvent CANCELLED by user ===";
                event->ignore();
                return;
            }
            qDebug() << "Tab" << i << "handled successfully";
        }
    }

    qDebug() << "=== closeEvent ACCEPTING - closing application ===";
    event->accept();
}

void MainWindow::on_actionNewFile_triggered() { createNewTab(); }

void MainWindow::handleSaveFileTriggered()
{
    EditorTabWidget *tab = currentEditorTab();
    if (!tab) return;
    tab->filePath().isEmpty() ? handleSaveFileAsTriggered()
                              : tab->saveFile(tab->filePath());
}

void MainWindow::handleTabModificationChanged(bool modified)
{
    EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(sender());
    if (tab) {
        int idx = ui->tabWidgetEditor->indexOf(tab);
        if (idx != -1) {
            QString title = QFileInfo(tab->filePath()).fileName();
            if (modified) title += "*";
            ui->tabWidgetEditor->setTabText(idx, title);
        }
    }
    ui->actionSave->setEnabled(modified);
    ui->actionSave_As->setEnabled(modified);
}

void MainWindow::handleTabTitleChanged(const QString &newTitle)
{
    EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(sender());
    if (tab) {
        int idx = ui->tabWidgetEditor->indexOf(tab);
        if (idx != -1)
            ui->tabWidgetEditor->setTabText(idx, newTitle);
    }
}

void MainWindow::saveProjectAs()
{
    QString initialPath = m_currentProjectFilePath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        : QFileInfo(m_currentProjectFilePath).absolutePath();

    QString saveFilePath = QFileDialog::getSaveFileName(
        this, tr("Save Project As"), initialPath,
        tr("Kayte IDE Project Files (*.xprj);;All Files (*.*)"));
    if (saveFilePath.isEmpty()) return;
    if (!saveFilePath.endsWith(".xprj", Qt::CaseInsensitive))
        saveFilePath += ".xprj";

    QFileInfo fileInfo(saveFilePath);
    m_currentProjectFilePath = saveFilePath;
    m_currentProjectName     = fileInfo.baseName();

    // Auto-set working directory for VC panels
    setCurrentProjectPath(fileInfo.absoluteDir().absolutePath());

    QFile file(saveFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error Saving Project"),
            tr("Cannot write file: %1\n%2").arg(saveFilePath, file.errorString()));
        return;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(4);
    xml.writeStartDocument();
    xml.writeStartElement("KayteIDEProject");
    xml.writeTextElement("ProjectName", m_currentProjectName);
    xml.writeTextElement("ProjectPath", fileInfo.absoluteDir().path());

    QString modeString;
    switch (currentDevelopmentMode) {
        case ChoiceMode::TextEditor: modeString = "Editor"; break;
        case ChoiceMode::RAD:        modeString = "RAD";    break;
        default:                     modeString = "Unknown";break;
    }
    xml.writeTextElement("DevelopmentMode", modeString);

    xml.writeStartElement("BuildSettings");
    xml.writeTextElement("BuildCommand", m_buildCommand);
    xml.writeTextElement("RunCommand",   m_runCommand);
    xml.writeTextElement("CleanCommand", m_cleanCommand);
    xml.writeTextElement("DebugCommand", m_debugCommand);
    xml.writeEndElement(); // BuildSettings

    xml.writeStartElement("OpenFiles");
    for (EditorTabWidget *tab : openEditorTabs) {
        if (!tab->filePath().isEmpty())
            xml.writeTextElement("File", tab->filePath());
    }
    xml.writeEndElement(); // OpenFiles

    xml.writeEndElement(); // KayteIDEProject
    xml.writeEndDocument();
    file.close();

    statusBar()->showMessage(tr("Project \"%1\" saved successfully.").arg(m_currentProjectName), 3000);
    qDebug() << "Project saved to:" << m_currentProjectFilePath;
}

void MainWindow::handleListViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QFileInfo fi = fileSystemModel->fileInfo(index);
    if (fi.isDir())
        setCurrentPath(fi.absoluteFilePath());
    else
        createNewTab(fi.absoluteFilePath());
}

bool MainWindow::handleSaveFileAsTriggered()
{
    EditorTabWidget *tab = currentEditorTab();
    if (!tab) return false;

    QString initial = tab->filePath().isEmpty() ? QDir::homePath() : tab->filePath();
    QString newPath = QFileDialog::getSaveFileName(
        this, tr("Save File As"), initial,
        tr("Text Files (*.txt *.vb *.cpp *.h *.kayte *.kyt *.pas *.pp *.dpr);;All Files (*.*)"));
    if (newPath.isEmpty()) return false;

    return tab->saveFile(newPath);
}

void MainWindow::on_actionCloseTab_triggered()
{
    int idx = ui->tabWidgetEditor->currentIndex();
    if (idx != -1)
        on_tabWidgetEditor_tabCloseRequested(idx);
}

void MainWindow::on_tabWidgetEditor_tabCloseRequested(int index)
{
    if (index < 0 || index >= ui->tabWidgetEditor->count()) return;
    EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(index));
    if (!tab) return;

    ui->tabWidgetEditor->setCurrentIndex(index);
    if (tab->isModified() && !saveCurrentFile()) return;

    openEditorTabs.removeOne(tab);
    ui->tabWidgetEditor->removeTab(index);
    tab->deleteLater();

    if (ui->tabWidgetEditor->count() == 0)
        createNewTab();
}

void MainWindow::updateTabTitle(bool modified)
{
    EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(sender());
    if (!tab) return;
    int idx = ui->tabWidgetEditor->indexOf(tab);
    if (idx != -1) {
        QString title = QFileInfo(tab->filePath()).fileName();
        if (title.isEmpty()) title = tr("Untitled");
        if (modified) title += "*";
        ui->tabWidgetEditor->setTabText(idx, title);
    }
}

void MainWindow::updateTabTitleOnRename(const QString &newTitle)
{
    EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(sender());
    if (!tab) return;
    int idx = ui->tabWidgetEditor->indexOf(tab);
    if (idx != -1) {
        QString title = newTitle;
        if (tab->isModified()) title += "*";
        ui->tabWidgetEditor->setTabText(idx, title);
    }
}

void MainWindow::buildProject()
{
    auto *t = currentEditorTab();
    QMessageBox::information(this, tr("Build"),
        t ? tr("Build triggered for: %1").arg(t->filePath())
          : tr("No active editor tab."));
}

void MainWindow::runProject()
{
    auto *t = currentEditorTab();
    QMessageBox::information(this, tr("Run"),
        t ? tr("Run triggered for: %1").arg(t->filePath())
          : tr("No active editor tab."));
}

void MainWindow::cleanProject()
{
    QMessageBox::information(this, tr("Clean"), tr("Clean project triggered."));
}

void MainWindow::debugProject()
{
    auto *t = currentEditorTab();
    QMessageBox::information(this, tr("Debug"),
        t ? tr("Debug triggered for: %1").arg(t->filePath())
          : tr("No active editor tab."));
}

void MainWindow::showAboutDialog()
{
    QMessageBox about(this);
    about.setWindowTitle(tr("About Kayte IDE"));
    about.setIconPixmap(QIcon::fromTheme("help-about").pixmap(64, 64));
    about.setTextFormat(Qt::RichText);
    about.setText(tr("about_dialog_text_html"));
    about.setStandardButtons(QMessageBox::Ok);
    about.exec();
}

void MainWindow::showModeSelectionDialog()
{
    ChoiceMode dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        currentDevelopmentMode = dialog.getSelectedMode();
        activateMode(currentDevelopmentMode);
    } else {
        QMessageBox::information(this, tr("Mode Selection"),
            tr("No mode selected. Defaulting to Text Editor Mode."));
        currentDevelopmentMode = ChoiceMode::TextEditor;
        activateMode(ChoiceMode::TextEditor);
    }
}

void MainWindow::activateMode(ChoiceMode::DevelopmentMode mode)
{
    qDebug() << "Activating mode:" << mode;

    auto *dlg = new DownloadProgressDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &DownloadProgressDialog::processCompleted,
            this, &MainWindow::handleDownloadDialogFinished);
    connect(dlg, &DownloadProgressDialog::processAborted,
            this, &MainWindow::handleDownloadDialogFinished);

    QStringList repos;
    if (mode == ChoiceMode::TextEditor) {
        QMessageBox::information(this, tr("Mode Activated"), tr("Text Editor Mode activated!"));
        ui->statusbar->showMessage(tr("Mode: Text Editor"), 3000);
        repos = editorModeRepos;
    } else if (mode == ChoiceMode::RAD) {
        QMessageBox::information(this, tr("Mode Activated"), tr("RAD Mode activated!"));
        ui->statusbar->showMessage(tr("Mode: RAD"), 3000);
        repos = radModeRepos;
    } else {
        qWarning() << "Unknown development mode.";
        ui->statusbar->showMessage(tr("Mode: Unknown"), 3000);
    }

    dlg->startProcess(repos, defaultDownloadPath);
    dlg->show();
}

void MainWindow::handleDownloadDialogFinished()
{
    QMessageBox::information(this, tr("Initialization Complete"), tr("IDE is ready!"));
    ui->statusbar->showMessage(tr("IDE Ready."), 3000);
}

void MainWindow::updateLineNumberAreaWidth(int /*newBlockCount*/) {}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

// ─────────────────────────────────────────────────────────────────────────────
// setupDownloadRepos  – FIXED
//
// Changes from the original:
//  1. rad_samples: removed the non-existent "--branch dev" override.
//     DownloadProgressDialog should clone the default branch (main/master).
//     If you need a specific branch when it exists, add a try/fallback in
//     DownloadProgressDialog itself rather than hard-coding "dev" here.
//  2. All repos now have the SAME skip-if-exists guard.  The format is
//     "URL;LOCAL_DIR_NAME" with an optional third field ";BRANCH" when you
//     are certain the branch exists on the remote.
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupDownloadRepos()
{
    // Format: "GIT_URL;LOCAL_DIR_NAME[;BRANCH]"
    // Omit the branch field if you want to clone the remote's default branch.
    radModeRepos
        << "https://github.com/ringsce/kayte-lang.git;kayte_lang;main"
        << "https://github.com/ringsce/rad-templates.git;rad_templates"
        << "https://github.com/ringsce/samples.git;rad_samples"          // ← removed ";dev" – branch doesn't exist
        << "https://github.com/ringsce/documentation.git;docs"
        << "https://github.com/ringsce/tutorials.git;tutorials";

    editorModeRepos
        << "https://github.com/ringsce/editor_addons.git;editor_addons";
}

void MainWindow::populateProjectList()
{
    ui->projectListWidget->clear();
    ui->projectListWidget->addItem(tr("My First RAD Project"));
    ui->projectListWidget->addItem(tr("Sample Game Engine"));
    ui->projectListWidget->addItem(tr("Business Application Prototype"));
    ui->projectListWidget->addItem(tr("Another Cool Project"));
}