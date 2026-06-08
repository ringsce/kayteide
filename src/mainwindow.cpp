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
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QToolButton>
#include <QApplication>
#include <QClipboard>
#include <QSysInfo>
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
#include "linenumberarea.h"
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

    // ── Application icon (bundled SVG, works on all platforms) ──────────
    setWindowIcon(QIcon(":/app-icon"));
    QApplication::setWindowIcon(QIcon(":/app-icon"));

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
    ui->actionNewFile->setIcon(QIcon::fromTheme("document-new", QIcon(":/icons/22/document-new")));
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open", QIcon(":/icons/22/document-open")));
    ui->actionSave->setIcon(QIcon::fromTheme("document-save", QIcon(":/icons/22/document-save")));
    ui->actionSave_As->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/icons/22/document-save-as")));
    ui->actionCloseTab->setIcon(QIcon::fromTheme("tab-close", QIcon(":/icons/22/tab-close")));
    ui->actionExit->setIcon(QIcon::fromTheme("application-exit", QIcon(":/icons/22/application-exit")));
    ui->actionBuild->setIcon(QIcon::fromTheme("system-run", QIcon(":/icons/22/system-run")));
    ui->actionClean->setIcon(QIcon::fromTheme("edit-clear", QIcon(":/icons/22/edit-clear")));
    ui->actionRun->setIcon(QIcon::fromTheme("media-playback-start", QIcon(":/icons/22/media-playback-start")));
    ui->actionDebug->setIcon(QIcon::fromTheme("tools-debugger", QIcon(":/icons/22/tools-debugger")));
    ui->actionAbout->setIcon(QIcon::fromTheme("help-about", QIcon(":/icons/22/help-about")));

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

    // ── About Qt ─────────────────────────────────────────────────────────────
    // Walk the menu bar to find the Help menu and append "About Qt".
    // QAction::AboutQtRole makes macOS move it into the application menu automatically.
    for (QAction *menuAction : menuBar()->actions()) {
        QMenu *m = menuAction->menu();
        if (m && menuAction->text().contains(tr("Help"), Qt::CaseInsensitive)) {
            m->addSeparator();
            QAction *aboutQtAct = m->addAction(
                QIcon::fromTheme("help-about", QIcon(":/icons/22/help-about")),
                tr("About &Qt"));
            aboutQtAct->setStatusTip(
                tr("Show information about the Qt framework version used by KayteIDE"));
            aboutQtAct->setMenuRole(QAction::AboutQtRole);
            connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
            break;
        }
    }
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

    // --- Project panel (left dock) ---
    setupProjectPanel();

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
        QIcon::fromTheme("svn", QIcon(":/icons/22/svn")),
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
        QIcon::fromTheme("git", QIcon(":/icons/22/git")),
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
        QIcon::fromTheme("folder", QIcon(":/icons/22/folder")),
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
        QIcon::fromTheme("preferences-desktop-keyboard", QIcon(":/icons/22/preferences-desktop-keyboard")),
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
    // Keep the project panel in sync whenever the active project changes.
    setProjectRoot(path);
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
        // Reset modified flag AFTER loading so opening a file doesn't
        // immediately trigger the "unsaved changes" dialog on close.
        newTab->setModified(false);
        int idx = ui->tabWidgetEditor->addTab(newTab, QFileInfo(filePath).fileName());
        ui->tabWidgetEditor->setCurrentIndex(idx);
        connect(newTab, &EditorTabWidget::modificationChanged, this, &MainWindow::updateTabTitle);
        connect(newTab, &EditorTabWidget::titleChanged, this, &MainWindow::updateTabTitleOnRename);
        connect(newTab, &EditorTabWidget::destroyed, this, &MainWindow::onTabClosed);

        // Attach line number area to files opened via File → Open
        if (QPlainTextEdit *ed = newTab->getPlainTextEdit()) {
            auto *lna = new LineNumberArea(ed, ed);
            const QPalette pal = ed->palette();
            if (pal.color(QPalette::Base).lightness() < 128) {
                lna->setBackgroundColor(pal.color(QPalette::Base).darker(110));
                lna->setForegroundColor(pal.color(QPalette::PlaceholderText));
                lna->setCurrentLineColor(pal.color(QPalette::Text));
            } else {
                lna->setBackgroundColor(pal.color(QPalette::Base).darker(105));
                lna->setForegroundColor(Qt::darkGray);
                lna->setCurrentLineColor(Qt::black);
            }
            lna->updateWidth(ed->blockCount());
        }
    } else {
        newTab->deleteLater();
    }
}

void MainWindow::on_tabWidgetEditor_currentChanged(int index)
{
    EditorTabWidget *current = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(index));
    m_keyboardShortcutsManager->setTargetEditor(current ? current->getPlainTextEdit() : nullptr);
}

// ═════════════════════════════════════════════════════════════════════════════
// Project Panel – left-side dock showing the open project's file tree
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::setupProjectPanel()
{
    // ── Dock widget ───────────────────────────────────────────────────────────
    m_projectDock = new QDockWidget(tr("Project"), this);
    m_projectDock->setObjectName(QStringLiteral("ProjectDock"));
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_projectDock->setFeatures(QDockWidget::DockWidgetMovable   |
                                QDockWidget::DockWidgetFloatable |
                                QDockWidget::DockWidgetClosable);

    // ── Container widget ──────────────────────────────────────────────────────
    auto *container = new QWidget(m_projectDock);
    auto *vlay      = new QVBoxLayout(container);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    // ── Header bar: project name + open-folder button ─────────────────────────
    auto *header    = new QWidget(container);
    header->setObjectName(QStringLiteral("ProjectPanelHeader"));
    header->setStyleSheet(
        QStringLiteral("QWidget#ProjectPanelHeader {"
                        "  background: palette(mid);"
                        "  border-bottom: 1px solid palette(dark);"
                        "}"));

    auto *headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(6, 4, 4, 4);
    headerLay->setSpacing(4);

    m_projectNameLabel = new QLabel(tr("(no project)"), header);
    m_projectNameLabel->setObjectName(QStringLiteral("ProjectNameLabel"));
    QFont lf = m_projectNameLabel->font();
    lf.setBold(true);
    m_projectNameLabel->setFont(lf);
    m_projectNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Open-folder button
    auto *openBtn = new QToolButton(header);
    openBtn->setIcon(QIcon::fromTheme("folder-new", QIcon(":/icons/22/folder-new")));
    openBtn->setToolTip(tr("Open project folder…"));
    openBtn->setAutoRaise(true);
    connect(openBtn, &QToolButton::clicked, this, &MainWindow::onOpenProjectFolder);

    // Collapse-all button
    auto *collapseBtn = new QToolButton(header);
    collapseBtn->setIcon(QIcon::fromTheme("view-list-tree",
                         QIcon(":/icons/22/view-split-left-right")));
    collapseBtn->setToolTip(tr("Collapse all"));
    collapseBtn->setAutoRaise(true);
    connect(collapseBtn, &QToolButton::clicked, this, &MainWindow::onCollapseAll);

    // Refresh button
    auto *refreshBtn = new QToolButton(header);
    refreshBtn->setIcon(QIcon::fromTheme("view-refresh",
                        QIcon(":/icons/22/vcs-update-required")));
    refreshBtn->setToolTip(tr("Refresh"));
    refreshBtn->setAutoRaise(true);
    connect(refreshBtn, &QToolButton::clicked, this, &MainWindow::onRefreshProject);

    headerLay->addWidget(m_projectNameLabel, 1);
    headerLay->addWidget(collapseBtn);
    headerLay->addWidget(refreshBtn);
    headerLay->addWidget(openBtn);

    // ── File-system model ─────────────────────────────────────────────────────
    m_projectModel = new QFileSystemModel(this);
    m_projectModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    // Show all source file types relevant to KayteIDE
    m_projectModel->setNameFilters({
        "*.kayte", "*.kyt",
        "*.cpp", "*.cxx", "*.cc", "*.c",
        "*.h", "*.hpp", "*.hxx",
        "*.vb",
        "*.pas", "*.pp", "*.dpr",
        "*.dfm",
        "*.txt", "*.md",
        "*.json", "*.xml",
        "*.xproj", "*.xprj",
        "CMakeLists.txt", "*.cmake",
        "Makefile", "*.mk",
        "*.ui",
    });
    m_projectModel->setNameFilterDisables(false); // hide non-matching files

    // Proxy for case-insensitive sorting
    m_projectProxy = new QSortFilterProxyModel(this);
    m_projectProxy->setSourceModel(m_projectModel);
    m_projectProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_projectProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    // ── Tree view ─────────────────────────────────────────────────────────────
    m_projectTree = new QTreeView(container);
    m_projectTree->setModel(m_projectProxy);
    m_projectTree->setRootIsDecorated(true);
    m_projectTree->setAnimated(true);
    m_projectTree->setUniformRowHeights(true);
    m_projectTree->setSortingEnabled(true);
    m_projectTree->sortByColumn(0, Qt::AscendingOrder);
    m_projectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_projectTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_projectTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_projectTree->setDragEnabled(false);
    m_projectTree->setHeaderHidden(true); // hide "Name / Size / Type / Date" header

    // Hide Size, Type, Date Modified columns – show only the name column
    m_projectTree->header()->setSectionHidden(1, true);
    m_projectTree->header()->setSectionHidden(2, true);
    m_projectTree->header()->setSectionHidden(3, true);
    m_projectTree->header()->setStretchLastSection(false);
    m_projectTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    connect(m_projectTree, &QTreeView::doubleClicked,
            this, &MainWindow::onProjectTreeDoubleClicked);
    connect(m_projectTree, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onProjectTreeContextMenu);

    // ── Assemble ──────────────────────────────────────────────────────────────
    vlay->addWidget(header);
    vlay->addWidget(m_projectTree, 1);

    m_projectDock->setWidget(container);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock);

    // ── View menu toggle action ───────────────────────────────────────────────
    m_actProjectPanel = m_projectDock->toggleViewAction();
    m_actProjectPanel->setText(tr("&Project Panel"));
    m_actProjectPanel->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    m_actProjectPanel->setStatusTip(tr("Show / hide the project file panel"));
    m_actProjectPanel->setIcon(
        QIcon::fromTheme("folder", QIcon(":/icons/22/folder")));

    // Add to View menu
    for (QAction *a : menuBar()->actions()) {
        if (a->menu() && a->text().contains(tr("View"), Qt::CaseInsensitive)) {
            a->menu()->insertAction(a->menu()->actions().isEmpty()
                                    ? nullptr : a->menu()->actions().first(),
                                    m_actProjectPanel);
            a->menu()->insertSeparator(a->menu()->actions().isEmpty()
                                       ? nullptr : a->menu()->actions().value(1));
            break;
        }
    }

    // Start at the home directory until a project is opened
    setProjectRoot(QDir::homePath());
}

// ─────────────────────────────────────────────────────────────────────────────
// setProjectRoot – point the tree at a new directory
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setProjectRoot(const QString &path)
{
    if (path.isEmpty() || !QDir(path).exists()) return;

    const QModelIndex srcRoot = m_projectModel->setRootPath(path);
    const QModelIndex proxyRoot = m_projectProxy->mapFromSource(srcRoot);
    m_projectTree->setRootIndex(proxyRoot);

    // Update the header label with the folder name
    const QString folderName = QDir(path).dirName();
    m_projectNameLabel->setText(folderName.isEmpty() ? path : folderName);
    m_projectNameLabel->setToolTip(path);

    // Expand the first level automatically
    m_projectTree->expandToDepth(0);

    // Raise the dock so the user can see it
    if (m_projectDock) {
        m_projectDock->show();
        m_projectDock->raise();
    }

    qDebug() << "[ProjectPanel] Root set to:" << path;
}

// ─────────────────────────────────────────────────────────────────────────────
// onOpenProjectFolder – browse for a folder and make it the project root
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onOpenProjectFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open Project Folder"),
        m_currentProjectPath.isEmpty() ? QDir::homePath() : m_currentProjectPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;

    setCurrentProjectPath(dir); // updates VC panels + project tree
}

// ─────────────────────────────────────────────────────────────────────────────
// onProjectTreeDoubleClicked – open the file in a new editor tab
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onProjectTreeDoubleClicked(const QModelIndex &proxyIndex)
{
    const QModelIndex srcIndex = m_projectProxy->mapToSource(proxyIndex);
    const QFileInfo fi = m_projectModel->fileInfo(srcIndex);

    if (fi.isDir()) {
        // Toggle expand/collapse on directories
        if (m_projectTree->isExpanded(proxyIndex))
            m_projectTree->collapse(proxyIndex);
        else
            m_projectTree->expand(proxyIndex);
        return;
    }

    if (fi.isFile()) {
        // Check if the file is already open in a tab
        for (int i = 0; i < ui->tabWidgetEditor->count(); ++i) {
            auto *tab = qobject_cast<EditorTabWidget *>(
                ui->tabWidgetEditor->widget(i));
            if (tab && tab->filePath() == fi.absoluteFilePath()) {
                ui->tabWidgetEditor->setCurrentIndex(i);
                return;
            }
        }
        createNewTab(fi.absoluteFilePath());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// onProjectTreeContextMenu – right-click menu on the project tree
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onProjectTreeContextMenu(const QPoint &pos)
{
    const QModelIndex proxyIndex = m_projectTree->indexAt(pos);
    const QModelIndex srcIndex   = m_projectProxy->mapToSource(proxyIndex);
    const QFileInfo fi = proxyIndex.isValid()
                         ? m_projectModel->fileInfo(srcIndex)
                         : QFileInfo();

    QMenu menu(this);

    if (fi.isFile()) {
        QAction *openAct = menu.addAction(
            QIcon::fromTheme("document-open", QIcon(":/icons/22/document-open")),
            tr("Open \"%1\"").arg(fi.fileName()));
        connect(openAct, &QAction::triggered, this, [this, fi]() {
            createNewTab(fi.absoluteFilePath());
        });

        menu.addSeparator();

        QAction *revealAct = menu.addAction(
            QIcon::fromTheme("folder", QIcon(":/icons/22/folder")),
            tr("Reveal in Finder / Explorer"));
        connect(revealAct, &QAction::triggered, this, [fi]() {
            // Cross-platform "reveal in file manager"
            QStringList args;
#if defined(Q_OS_MACOS)
            args << "-e" << "tell application \"Finder\" to reveal POSIX file \"" +
                             fi.absoluteFilePath() + "\"";
            QProcess::startDetached("osascript", args);
            QProcess::startDetached("open", {"-R", fi.absoluteFilePath()});
#elif defined(Q_OS_WIN)
            args << "/select," << QDir::toNativeSeparators(fi.absoluteFilePath());
            QProcess::startDetached("explorer.exe", args);
#else
            QProcess::startDetached("xdg-open", {fi.absolutePath()});
#endif
        });

        menu.addSeparator();

        QAction *copyPathAct = menu.addAction(tr("Copy Full Path"));
        connect(copyPathAct, &QAction::triggered, this, [fi]() {
            QApplication::clipboard()->setText(fi.absoluteFilePath());
        });

    } else if (fi.isDir()) {
        QAction *setRootAct = menu.addAction(
            QIcon::fromTheme("folder-new", QIcon(":/icons/22/folder-new")),
            tr("Set as Project Root"));
        connect(setRootAct, &QAction::triggered, this, [this, fi]() {
            setCurrentProjectPath(fi.absoluteFilePath());
        });

        menu.addSeparator();

        QAction *newFileAct = menu.addAction(
            QIcon::fromTheme("document-new", QIcon(":/icons/22/document-new")),
            tr("New File Here…"));
        connect(newFileAct, &QAction::triggered, this, [this, fi]() {
            bool ok = false;
            const QString name = QInputDialog::getText(
                this, tr("New File"),
                tr("File name:"), QLineEdit::Normal,
                QStringLiteral("newfile.kayte"), &ok).trimmed();
            if (!ok || name.isEmpty()) return;
            const QString fullPath = fi.absoluteFilePath() + "/" + name;
            QFile f(fullPath);
            if (f.open(QIODevice::WriteOnly)) {
                f.close();
                createNewTab(fullPath);
                onRefreshProject();
            } else {
                QMessageBox::warning(this, tr("Error"),
                    tr("Could not create file:\n%1").arg(fullPath));
            }
        });
    }

    // Always show "Open Folder…" at the bottom
    menu.addSeparator();
    QAction *openFolderAct = menu.addAction(
        QIcon::fromTheme("folder", QIcon(":/icons/22/folder")),
        tr("Open Project Folder…"));
    connect(openFolderAct, &QAction::triggered, this, &MainWindow::onOpenProjectFolder);

    menu.exec(m_projectTree->viewport()->mapToGlobal(pos));
}

// ─────────────────────────────────────────────────────────────────────────────
// onCollapseAll / onRefreshProject
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onCollapseAll()
{
    m_projectTree->collapseAll();
    m_projectTree->expandToDepth(0); // keep the root level visible
}

void MainWindow::onRefreshProject()
{
    // QFileSystemModel updates automatically, but we can force-reload the root
    const QString current = m_projectModel->rootPath();
    m_projectModel->setRootPath(QString()); // reset
    setProjectRoot(current);                // re-apply
    statusBar()->showMessage(tr("Project tree refreshed."), 2000);
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
    // If the user asks for a NEW Untitled tab and the current tab is already
    // an empty, unmodified Untitled tab, reuse it instead of stacking blanks.
    if (filePath.isEmpty() && ui->tabWidgetEditor->count() > 0) {
        EditorTabWidget *current = currentEditorTab();
        if (current && current->filePath().isEmpty() && !current->isModified()) {
            // Current tab is a pristine Untitled – nothing to do; just focus it.
            return;
        }
        if (current && current->filePath().isEmpty() && current->isModified()) {
            // Current tab has unsaved Untitled content – ask before replacing.
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

    // Do NOT mark a fresh Untitled tab as modified.
    // isModified() stays false until the user actually types something,
    // so closeTab won't ask "save changes?" for an untouched new file.
    if (!filePath.isEmpty()) {
        // Reset the modified flag after loading so the "save?" dialog
        // doesn't fire immediately after opening a file from disk.
        editorTab->setModified(false);
    }
    // ── Line number area ─────────────────────────────────────────────────────
    // EditorTabWidget::getPlainTextEdit() should return a CodeEditor* so that
    // LineNumberArea::updateWidth() can call setViewportMargins() (protected).
    // We cast to CodeEditor*; if the editor is still a plain QPlainTextEdit
    // the gutter still paints correctly — only the margin won't auto-set.
    if (QPlainTextEdit *ed = editorTab->getPlainTextEdit()) {
        auto *lna = new LineNumberArea(ed, ed);
        const QPalette pal = ed->palette();
        const QColor   base = pal.color(QPalette::Base);
        if (base.lightness() < 128) {
            lna->setBackgroundColor(base.darker(110));
            lna->setForegroundColor(pal.color(QPalette::PlaceholderText));
            lna->setCurrentLineColor(pal.color(QPalette::Text));
        } else {
            lna->setBackgroundColor(base.darker(105));
            lna->setForegroundColor(Qt::darkGray);
            lna->setCurrentLineColor(Qt::black);
        }
        lna->updateWidth(ed->blockCount());
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

    bool ok = tab->filePath().isEmpty()
              ? handleSaveFileAsTriggered()           // Untitled → Save As dialog
              : tab->saveFile(tab->filePath());       // known path → save in-place

    if (ok)
        statusBar()->showMessage(tr("File saved"), 2000);
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

    // Only prompt when there are actual unsaved edits.
    // An Untitled tab that was never typed into (isModified==false) closes silently.
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

    QIcon appIcon(":/app-icon");
    if (appIcon.isNull())
        appIcon = QIcon::fromTheme("help-about", QIcon(":/icons/22/help-about"));
    about.setIconPixmap(appIcon.pixmap(64, 64));

    about.setTextFormat(Qt::RichText);
    about.setText(
        tr("<h2>Kayte IDE</h2>"
           "<p>Version %1 &nbsp;|&nbsp; Built with Qt %2</p>"
           "<p>KayteIDE is an open-source integrated development environment "
           "for the <b>Kayte</b> programming language, with support for "
           "C++, Pascal, Delphi, and Visual Basic syntax.</p>"
           "<p>"
           "<a href=\"https://github.com/ringsce/kayteide\">GitHub</a>"
           " &nbsp;&middot;&nbsp; "
           "<a href=\"https://ringscejs.gleentech.com\">ringsce.com</a>"
           "</p>"
           "<p style=\"font-size:small; color:gray;\">"
           "Copyright &copy; 2024&ndash;2026 ringsce. "
           "Released under the MIT Licence."
           "</p>")
        .arg(QApplication::applicationVersion(),
             QString::fromLatin1(qVersion()))
    );
    about.setInformativeText(
        QString("Qt %1 \xc2\xb7 %2 \xc2\xb7 %3")
        .arg(QString::fromLatin1(qVersion()),
#if defined(Q_OS_MACOS)
             QStringLiteral("macOS"),
#elif defined(Q_OS_WIN)
             QStringLiteral("Windows"),
#else
             QStringLiteral("Linux"),
#endif
             QSysInfo::currentCpuArchitecture())
    );
    about.setStandardButtons(QMessageBox::Ok);
    about.setDefaultButton(QMessageBox::Ok);
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

void MainWindow::updateLineNumberAreaWidth(int newBlockCount)
{
    // Delegate to the active tab's LineNumberArea if it has one.
    // EditorTabWidget exposes getPlainTextEdit(); the LineNumberArea is
    // installed as a child of the editor, so we can find it by type.
    Q_UNUSED(newBlockCount)
    if (EditorTabWidget *tab = currentEditorTab()) {
        if (QPlainTextEdit *ed = tab->getPlainTextEdit()) {
            if (auto *lna = ed->findChild<LineNumberArea *>()) {
                lna->updateWidth(newBlockCount);
            }
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    // Resize the LineNumberArea of the active tab to match the new geometry.
    if (EditorTabWidget *tab = currentEditorTab()) {
        if (QPlainTextEdit *ed = tab->getPlainTextEdit()) {
            if (auto *lna = ed->findChild<LineNumberArea *>()) {
                lna->updateWidth(ed->blockCount());
            }
        }
    }
}

void MainWindow::updateLineNumberArea(const QRect &rect, int dy)
{
    // Forwarded from EditorTabWidget's updateRequest signal.
    // The LineNumberArea's own onUpdateRequest slot handles this;
    // this method exists for compatibility with the header declaration.
    Q_UNUSED(rect) Q_UNUSED(dy)
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