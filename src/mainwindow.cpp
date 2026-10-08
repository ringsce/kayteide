#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "welcometabwidget.h"

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
#include <QSettings>
#include <QCryptographicHash>
#include <QPainter>
#include <iostream>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QInputDialog>
#include <QToolBar>
#include <QTimer>
#include <QRegularExpression>
#include <QActionGroup>
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
#  define ICON_FA_ROBOT      "\xef\x95\x84"  // U+F544
#  define KAYTEIDE_FA_AVAILABLE 0
#endif

// Custom widgets and dialogs
#include "editortabwidget.h"
#include "linenumberarea.h"
#include "keyboard.h"
#include "bottompanel.h"
#include "modebar.h"
#include "buildconfigdialog.h"
#include "toolchainsetupdialog.h"
#include "largefileview.h"
#include "AssistantPanel.hpp"   // from plugins/llm/include/

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

    // Asynchronous start: never block the GUI thread waiting for the shell.
    connect(m_process, &QProcess::started, this, [this] {
        appendOutput(QStringLiteral("[bash] ready\n"));
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            appendOutput(QStringLiteral("[ERROR] Could not start /bin/bash\n"), true);
    });
    m_process->start(QStringLiteral("/bin/bash"),
                     QStringList() << QStringLiteral("--norc") << QStringLiteral("-i"));
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

    showWelcomeTab();

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
    applyFontAwesomeIcons();
    setupTerminalDock();

    // --- Widget Palette + UI Designer ---
    setupWidgetPalette();

    // --- Project panel (left dock) ---
    setupProjectPanel();

    // --- Local LLM assistant (right dock) ---
    setupAssistantDock();

    // --- Build configurations (Debug / Release / …) of the current project ---
    m_buildConfigs = new BuildConfigurations(this);
    connect(m_buildConfigs, &BuildConfigurations::changed, this, &MainWindow::updateModeBarKit);
    ui->menuProject->addSeparator();
    QMenu *configMenu = ui->menuProject->addMenu(tr("Build &Configuration"));
    connect(configMenu, &QMenu::aboutToShow, this,
            [this, configMenu] { populateBuildConfigMenu(configMenu); });

    // --- Qt Creator-style mode bar (far left) ---
    setupModeBar();
    ensureBuildConfigs();

    // --- First launch: install the toolchain (FPC, Lazarus, Kayte SDK, QEMU) ---
    QTimer::singleShot(1000, this, &MainWindow::maybeRunFirstSetup);

#ifdef Q_OS_MACOS
    // Qt 6.11 on macOS 27 crashes (SIGTRAP in QImage::toCGImage) whenever it
    // builds a cursor macOS has no native version of – QTBUG-150017, not fixed
    // in 6.11.2. A toolbar's drag handle shows such a cursor (SizeAll) on
    // hover, so main-window toolbars are fixed in place on macOS.
    for (QToolBar *tb : findChildren<QToolBar *>())
        if (tb->parentWidget() == this) tb->setMovable(false);
#endif
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

    // ── Components editor (left, tabbed with the palette) ─────────────────────
    m_componentsDock = new ComponentsEditorDock(m_canvas, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_componentsDock);
    tabifyDockWidget(m_paletteDock, m_componentsDock);
    m_componentsDock->hide();

    // ── Property editor (right, next to the designer canvas) ──────────────────
    m_propertyDock = new PropertyEditorDock(m_canvas, this);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);
    splitDockWidget(m_designerDock, m_propertyDock, Qt::Horizontal);
    m_propertyDock->hide();

    // ── Toolbar buttons ───────────────────────────────────────────────────────
    QToolBar *tb = addToolBar(tr("UI Designer"));
    tb->setObjectName(QStringLiteral("UiDesignerToolBar"));

#if __has_include(<IconsFontAwesome6.h>)
    m_actPalette  = new QAction(faIcon(ICON_FA_OBJECT_GROUP), tr("Widget &Palette"), this);
    m_actDesigner = new QAction(faIcon(ICON_FA_IMAGE),        tr("UI &Designer"), this);
#else
    m_actPalette  = new QAction(tr("Widget &Palette"), this);
    m_actDesigner = new QAction(tr("UI &Designer"), this);
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

#if __has_include(<IconsFontAwesome6.h>)
    m_actComponents = new QAction(faIcon(ICON_FA_SITEMAP), tr("&Components"), this);
    m_actProperties = new QAction(faIcon(ICON_FA_SLIDERS), tr("P&roperties"), this);
#else
    m_actComponents = new QAction(tr("&Components"), this);
    m_actProperties = new QAction(tr("P&roperties"), this);
#endif

    m_actComponents->setToolTip(tr("Toggle Components editor"));
    m_actComponents->setStatusTip(tr("Show / hide the list of components on the current form"));
    m_actComponents->setCheckable(true);
    m_actComponents->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(m_actComponents, &QAction::triggered, this, &MainWindow::onToggleWidgetPalette);
    tb->addAction(m_actComponents);

    m_actProperties->setToolTip(tr("Toggle Property editor"));
    m_actProperties->setStatusTip(tr("Show / hide the property editor for the selected component"));
    m_actProperties->setCheckable(true);
    m_actProperties->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    connect(m_actProperties, &QAction::triggered, this, &MainWindow::onToggleWidgetPalette);
    tb->addAction(m_actProperties);

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
    viewMenu->addAction(m_actComponents);
    viewMenu->addAction(m_actProperties);

    // Keep actions in sync with dock visibility
    connect(m_paletteDock,    &QDockWidget::visibilityChanged, m_actPalette,    &QAction::setChecked);
    connect(m_designerDock,   &QDockWidget::visibilityChanged, m_actDesigner,   &QAction::setChecked);
    connect(m_componentsDock, &QDockWidget::visibilityChanged, m_actComponents, &QAction::setChecked);
    connect(m_propertyDock,   &QDockWidget::visibilityChanged, m_actProperties, &QAction::setChecked);
}

void MainWindow::onToggleWidgetPalette()
{
    const bool showPalette    = m_actPalette     ? m_actPalette->isChecked()     : !m_paletteDock->isVisible();
    const bool showDesigner   = m_actDesigner    ? m_actDesigner->isChecked()    : !m_designerDock->isVisible();
    const bool showComponents = m_actComponents  ? m_actComponents->isChecked()  : !m_componentsDock->isVisible();
    const bool showProperties = m_actProperties  ? m_actProperties->isChecked()  : !m_propertyDock->isVisible();
    m_paletteDock->setVisible(showPalette);
    m_designerDock->setVisible(showDesigner);
    m_componentsDock->setVisible(showComponents);
    m_propertyDock->setVisible(showProperties);
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
    // Show all designer docks so the user can start designing immediately
    if (m_paletteDock)    { m_paletteDock->show();    m_paletteDock->raise(); }
    if (m_designerDock)   { m_designerDock->show();   m_designerDock->raise(); }
    if (m_componentsDock) { m_componentsDock->show(); m_componentsDock->raise(); }
    if (m_propertyDock)   { m_propertyDock->show();   m_propertyDock->raise(); }
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

// Renders a Font Awesome glyph in the current text colour, so icons stay
// visible on both light and dark themes. Returns a null icon if the font
// could not be loaded (callers then keep their theme / resource icon).
QIcon MainWindow::faIcon(const char *glyph) const
{
    if (!QFontDatabase::families().contains(m_faFont.family()))
        return {};

    QIcon icon;
    const QColor normal   = palette().color(QPalette::Active,   QPalette::WindowText);
    const QColor disabled = palette().color(QPalette::Disabled, QPalette::WindowText);
    for (int px : { 16, 22, 32, 44, 64 }) {
        for (const auto &[mode, color] : { std::pair { QIcon::Normal,   normal },
                                           std::pair { QIcon::Disabled, disabled } }) {
            QPixmap pm(px, px);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::TextAntialiasing);
            QFont f = m_faFont;
            f.setPixelSize(qRound(px * 0.78));
            p.setFont(f);
            p.setPen(color);
            p.drawText(pm.rect(), Qt::AlignCenter, QString::fromUtf8(glyph));
            icon.addPixmap(pm, mode);
        }
    }
    return icon;
}

// One consistent icon set for the main toolbar and menus. Every action shows
// an image instead of its name, and none disappear on a dark theme.
void MainWindow::applyFontAwesomeIcons()
{
#if KAYTEIDE_FA_AVAILABLE
    const std::pair<QAction *, const char *> icons[] = {
        { ui->actionNewFile,       ICON_FA_FILE },
        { ui->actionOpen,          ICON_FA_FOLDER_OPEN },
        { ui->actionSave,          ICON_FA_FLOPPY_DISK },
        { ui->actionSave_As,       ICON_FA_PEN_TO_SQUARE },
        { ui->actionSaveProjectAs, ICON_FA_FILE_EXPORT },
        { ui->actionNewProject,    ICON_FA_FOLDER_PLUS },
        { ui->actionCloseTab,      ICON_FA_XMARK },
        { ui->actionCut,           ICON_FA_SCISSORS },
        { ui->actionCopy,          ICON_FA_COPY },
        { ui->actionPaste,         ICON_FA_PASTE },
        { ui->actionSelectAll,     ICON_FA_CHECK_DOUBLE },
        { ui->actionBuild,         ICON_FA_HAMMER },
        { ui->actionClean,         ICON_FA_BROOM },
        { ui->actionRun,           ICON_FA_PLAY },
        { ui->actionDebug,         ICON_FA_BUG },
        { ui->actionAbout,         ICON_FA_CIRCLE_INFO },
        { ui->actionExit,          ICON_FA_RIGHT_FROM_BRACKET },
    };
    for (const auto &[action, glyph] : icons) {
        const QIcon icon = faIcon(glyph);
        if (!icon.isNull()) action->setIcon(icon);
    }
#endif
    // Names stay in tooltips; the toolbar itself shows images only.
    ui->toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
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

    // ── VSCodium-style panel: PROBLEMS · OUTPUT · DEBUG CONSOLE · TERMINAL · PORTS
    // The panel draws its own tab strip, so the dock gets an empty title bar.
    m_terminalDock->setTitleBarWidget(new QWidget(m_terminalDock));

    m_bottomPanel    = new BottomPanel(m_terminalDock);
    m_terminalWidget = m_bottomPanel->currentTerminal();
    m_terminalDock->setWidget(m_bottomPanel);
    m_terminalDock->setMinimumHeight(160);

    connect(m_bottomPanel, &BottomPanel::closeRequested,
            m_terminalDock, &QDockWidget::hide);
    connect(m_bottomPanel, &BottomPanel::maximizeToggled, this, [this](bool on) {
        if (centralWidget()) centralWidget()->setVisible(!on);
    });
    connect(m_bottomPanel, &BottomPanel::problemActivated, this,
            [this](const QString &file, int line, int column) {
        openFileAtLine(file, line, column);
    });

    // ── Build diagnostics → bug markers in the editors ───────────────────────
    connect(m_bottomPanel, &BottomPanel::problemAdded, this,
            [this](const QString &file, int line, int column,
                   const QString &severity, const QString &message) {
        if (line <= 0) return;
        const QString key = diagnosticKey(file);
        m_diagnostics[key].append({line, column, severity, message});
        for (EditorTabWidget *tab : std::as_const(openEditorTabs))
            if (diagnosticKey(tab->filePath()) == key)
                applyDiagnostics(tab);
    });
    connect(m_bottomPanel, &BottomPanel::problemsCleared, this, [this] {
        m_diagnostics.clear();
        for (EditorTabWidget *tab : std::as_const(openEditorTabs))
            tab->setDiagnostics({});
    });
    connect(m_bottomPanel, &BottomPanel::taskFinished, this, &MainWindow::onTaskFinished);

    // Stack in the same bottom area as SVN / Git docks
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);
    if (m_gitDock)
        tabifyDockWidget(m_gitDock, m_terminalDock);

    // ── Toolbar: Font Awesome "terminal" glyph + "folder-plus" (.xproj) ───────
    QToolBar *tb = addToolBar(tr("Terminal / Project"));
    tb->setObjectName(QStringLiteral("TerminalToolBar"));

    // Terminal toggle button
    m_actTerminal = new QAction(faIcon(ICON_FA_TERMINAL), tr("&Panel (Terminal, Output, Problems…)"), this);
    m_actTerminal->setToolTip(tr("Toggle Panel  (Ctrl+`)"));
    m_actTerminal->setStatusTip(tr("Show / hide Problems, Output, Debug Console, Terminal and Ports"));
    m_actTerminal->setCheckable(true);
    m_actTerminal->setChecked(true);
    m_actTerminal->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
    connect(m_actTerminal, &QAction::triggered, this, &MainWindow::onToggleTerminal);
    tb->addAction(m_actTerminal);

    // .xproj scaffold button
    auto *actXProj = new QAction(faIcon(ICON_FA_FOLDER_PLUS), tr("New .xproj Project"), this);
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
// Local LLM assistant dock (plugins/llm)
// A chat panel backed by Ollama that learns the open project's C, C++, Kayte,
// VB, BASIC and CMake sources. Sessions are saved under a short hash.
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupAssistantDock()
{
    m_assistant = new Kayte::Llm::AssistantPanel(this);
    m_assistant->setCurrentFileProvider([this]() -> QPair<QString, QString> {
        EditorTabWidget *t = currentEditorTab();
        if (!t) return {};
        return { t->filePath(), t->getPlainTextEdit()->toPlainText() };
    });
    connect(m_assistant, &Kayte::Llm::AssistantPanel::statusMessage, this,
            [this](const QString &msg) { statusBar()->showMessage(msg, 5000); });
    connect(m_assistant, &Kayte::Llm::AssistantPanel::openFileRequested, this,
            [this](const QString &file, int line) { openFileAtLine(file, line); });

    m_assistantDock = new QDockWidget(tr("Kayte Assistant"), this);
    m_assistantDock->setObjectName(QStringLiteral("AssistantDock"));
    m_assistantDock->setWidget(m_assistant);
    m_assistantDock->setMinimumWidth(320);
    addDockWidget(Qt::RightDockWidgetArea, m_assistantDock);

    // The dock's own toggle action stays in sync with its visibility, whether
    // it is closed from the bar, the menu, the shortcut or its title bar.
    QAction *act = m_assistantDock->toggleViewAction();
    act->setText(tr("Kayte &Assistant (Local LLM)"));
    act->setIconText(tr("LLM"));
    act->setToolTip(tr("Show / hide Kayte Assistant  (Ctrl+Shift+L)"));
    act->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));

    // Font Awesome robot; a hand-drawn robot if the icon font is unavailable.
    if (const QIcon fa = faIcon(ICON_FA_ROBOT); !fa.isNull()) {
        act->setIcon(fa);
    } else {
        QPixmap pm(48, 48);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor fg = palette().color(QPalette::WindowText);
        p.setPen(QPen(fg, 3));
        p.setBrush(Qt::NoBrush);
        p.drawLine(24, 6, 24, 12);                       // antenna
        p.setBrush(fg);
        p.drawEllipse(QPointF(24, 5), 2.5, 2.5);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(9, 13, 30, 24), 6, 6);  // head
        p.drawLine(4, 22, 4, 30);                        // ears
        p.drawLine(44, 22, 44, 30);
        p.setPen(Qt::NoPen);
        p.setBrush(fg);
        p.drawEllipse(QPointF(18, 23), 3.5, 3.5);        // eyes
        p.drawEllipse(QPointF(30, 23), 3.5, 3.5);
        p.drawRoundedRect(QRectF(17, 30, 14, 3), 1.5, 1.5); // mouth
        act->setIcon(QIcon(pm));
    }

    // ── Activity bar pinned to the right edge: LLM toggle ────────────────────
    auto *llmBar = new QToolBar(tr("Assistant Bar"), this);
    llmBar->setObjectName(QStringLiteral("AssistantActivityBar"));
    llmBar->setAllowedAreas(Qt::RightToolBarArea);
    llmBar->setMovable(false);
    llmBar->setFloatable(false);
    llmBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    llmBar->setIconSize(QSize(24, 24));
    llmBar->addAction(act);
    addToolBar(Qt::RightToolBarArea, llmBar);
    for (QAction *a : menuBar()->actions()) {
        if (a->menu() && a->text().contains(tr("View"), Qt::CaseInsensitive)) {
            a->menu()->addSeparator();
            a->menu()->addAction(act);
            break;
        }
    }

    if (!m_currentProjectPath.isEmpty())
        m_assistant->setProjectRoot(m_currentProjectPath);
}

bool MainWindow::resumeAssistantSession(const QString &hash, QString *error)
{
    if (!m_assistant || !m_assistant->resumeSession(hash, error))
        return false;
    m_assistantDock->show();
    m_assistantDock->raise();
    // Bring back the project the session was about.
    const QString root = m_assistant->sessionProject();
    if (!root.isEmpty() && QFileInfo(root).isDir() && root != m_currentProjectPath)
        setCurrentProjectPath(root);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Toggle Terminal
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onToggleTerminal()
{
    const bool show = !m_terminalDock->isVisible();
    m_terminalDock->setVisible(show);
    if (show) m_bottomPanel->showView(BottomPanel::Terminal);
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

    // 5. Write a starter main.kayte (BASIC-style Kayte, built by the bundled SDK)
    QFile mainKayte(projRoot + QStringLiteral("/src/main.kayte"));
    if (mainKayte.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString shown = projectName;
        shown.replace(QLatin1Char('"'), QLatin1Char('\''));
        QTextStream ts(&mainKayte);
        ts << QStringLiteral("' ") << projectName
           << QStringLiteral(" - created with KayteIDE\n")
           << QStringLiteral("'   Run it with the Run button, or: kayte run\n\n")
           << QStringLiteral("name = \"") << shown << QStringLiteral("\"\n")
           << QStringLiteral("PRINT \"Hello from \" & name & \"!\"\n");
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

    QAction *gitAction = m_actGit = toolsMenu->addAction(
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

    // ── Toolchain installer (bundled requirements.sh) ────────────────────────
    QAction *setupAction = toolsMenu->addAction(tr("Install / Update &Toolchain…"));
    setupAction->setStatusTip(tr("Install or update Free Pascal, Lazarus, the Kayte SDK and QEMU "
                                 "inside KayteIDE.app"));
    setupAction->setEnabled(!ToolchainSetupDialog::installerScript().isEmpty());
    connect(setupAction, &QAction::triggered, this, [this] { showToolchainSetup(false); });
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

    // ── Updater ───────────────────────────────────────────────────────────────
    QAction *updaterAction = toolsMenu->addAction(
        QIcon::fromTheme("system-software-update", QIcon(":/icons/22/vcs-update-required")),
        tr("&Updater…"));
    updaterAction->setStatusTip(tr("Launch KayteIDEUpdater to check for and build updates"));
    connect(updaterAction, &QAction::triggered, this, &MainWindow::launchUpdater);

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
    if (m_assistant)
        m_assistant->setProjectRoot(path);   // re-learn the new project's code
    if (m_buildConfigs) ensureBuildConfigs();
    updateModeBarKit();
}

// ─────────────────────────────────────────────────────────────────────────────
// Updater launcher
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::launchUpdater()
{
    // KayteIDEUpdater is built and placed right alongside KayteIDE itself:
    // on macOS it's embedded in the same .app/Contents/MacOS directory (see
    // the POST_BUILD step in CMakeLists.txt), and on Linux/dev builds both
    // executables land in the same RUNTIME_OUTPUT_DIRECTORY (build/bin, or
    // CMAKE_INSTALL_BINDIR once packaged) — so applicationDirPath() finds it
    // in every case without needing platform-specific bundle logic here.
    const QString updaterName = QStringLiteral("KayteIDEUpdater")
#ifdef Q_OS_WIN
        + QStringLiteral(".exe")
#endif
        ;
    const QString updaterPath = QDir(QCoreApplication::applicationDirPath())
                                     .filePath(updaterName);

    if (!QFile::exists(updaterPath)) {
        QMessageBox::warning(this, tr("Updater Not Found"),
            tr("Could not find KayteIDEUpdater at:\n%1\n\n"
               "Make sure the KayteIDEUpdater target has been built.")
                .arg(updaterPath));
        return;
    }

    if (!QProcess::startDetached(updaterPath, {})) {
        QMessageBox::warning(this, tr("Updater Failed to Start"),
            tr("Failed to launch KayteIDEUpdater at:\n%1").arg(updaterPath));
        return;
    }

    statusBar()->showMessage(tr("Launched KayteIDEUpdater"), 4000);
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
           ";;Markdown (*.md *.markdown)"
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
        applyDiagnostics(newTab);
    } else {
        newTab->deleteLater();
    }
}

void MainWindow::on_tabWidgetEditor_currentChanged(int index)
{
    EditorTabWidget *current = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(index));
    m_keyboardShortcutsManager->setTargetEditor(current ? current->getPlainTextEdit() : nullptr);

    // Keep the mode bar in step: the Welcome page is its own mode, and
    // switching to a document from it means editing.
    if (m_modeBar) {
        const bool welcome = m_welcomeTab && ui->tabWidgetEditor->widget(index) == m_welcomeTab;
        if (welcome)
            m_modeBar->setCurrentMode(m_modeWelcome);
        else if (m_modeBar->currentMode() == m_modeWelcome)
            m_modeBar->setCurrentMode(m_modeEdit);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Mode bar (Qt Creator style): Welcome / Edit / Design / Debug modes, panel
// buttons, the current project and Run / Debug / Build at the bottom.
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::setupModeBar()
{
    m_modeBar = new ModeBar(this);
    m_modeWelcome = m_modeBar->addMode(ModeBar::Home,   tr("Welcome"), tr("Welcome page"));
    m_modeEdit    = m_modeBar->addMode(ModeBar::Edit,   tr("Edit"),    tr("Edit code"));
    m_modeDesign  = m_modeBar->addMode(ModeBar::Design, tr("Design"),  tr("Design forms (RAD)"));
    m_modeDebug   = m_modeBar->addMode(ModeBar::Debug,  tr("Debug"),   tr("Debug console"));
    m_btnProjects = m_modeBar->addButton(ModeBar::Projects, tr("Projects"), tr("Show / hide the project panel"));
    m_btnGit      = m_modeBar->addButton(ModeBar::Git,      tr("Git"),      tr("Show / hide the Git panel"));
    m_btnHelp     = m_modeBar->addButton(ModeBar::Help,     tr("Help"),     tr("About KayteIDE"));

    m_modeBar->addBottomAction(ui->actionRun,   ModeBar::Run);
    m_modeBar->addBottomAction(ui->actionDebug, ModeBar::DebugRun);
    m_modeBar->addBottomAction(ui->actionBuild, ModeBar::Build);

    connect(m_modeBar, &ModeBar::itemActivated, this, &MainWindow::onModeBarItem);
    connect(m_modeBar, &ModeBar::kitClicked,    this, &MainWindow::showKitMenu);

    // Run / Debug / Build now live on the mode bar, as in Qt Creator.
    ui->toolBar->removeAction(ui->actionBuild);
    ui->toolBar->removeAction(ui->actionRun);
    ui->toolBar->removeAction(ui->actionDebug);

    auto *bar = new QToolBar(tr("Mode Bar"), this);
    bar->setObjectName(QStringLiteral("ModeBarToolBar"));
    bar->setAllowedAreas(Qt::LeftToolBarArea);
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->toggleViewAction()->setVisible(false);
    bar->setContentsMargins(0, 0, 0, 0);
    bar->layout()->setContentsMargins(0, 0, 0, 0);
    bar->layout()->setSpacing(0);
    bar->setStyleSheet(QStringLiteral(
        "QToolBar#ModeBarToolBar { border: none; padding: 0; margin: 0; spacing: 0; }"));
    bar->addWidget(m_modeBar);
    addToolBar(Qt::LeftToolBarArea, bar);

    const QWidget *cur = ui->tabWidgetEditor->currentWidget();
    m_modeBar->setCurrentMode(m_welcomeTab && cur == m_welcomeTab ? m_modeWelcome : m_modeEdit);
}

void MainWindow::onModeBarItem(int index)
{
    const auto showDesigner = [this](bool show) {
        m_paletteDock->setVisible(show);
        m_designerDock->setVisible(show);
        m_componentsDock->setVisible(show);
        m_propertyDock->setVisible(show);
    };

    if (index == m_modeWelcome) {
        showDesigner(false);
        showWelcomeTab();
    } else if (index == m_modeEdit) {
        currentDevelopmentMode = DevelopmentMode::TextEditor;
        showDesigner(false);
        m_projectDock->show();
        // Leave the Welcome page for the most recent document, if any.
        if (m_welcomeTab && ui->tabWidgetEditor->currentWidget() == m_welcomeTab) {
            for (int i = ui->tabWidgetEditor->count() - 1; i >= 0; --i) {
                if (ui->tabWidgetEditor->widget(i) != m_welcomeTab) {
                    ui->tabWidgetEditor->setCurrentIndex(i);
                    break;
                }
            }
        }
        if (EditorTabWidget *ed = currentEditorTab())
            ed->getPlainTextEdit()->setFocus();
    } else if (index == m_modeDesign) {
        currentDevelopmentMode = DevelopmentMode::RAD;   // recorded in saved projects
        showDesigner(true);
        m_designerDock->raise();
    } else if (index == m_modeDebug) {
        showDesigner(false);
        m_terminalDock->show();
        m_terminalDock->raise();
        m_bottomPanel->showView(BottomPanel::DebugConsole);
    } else if (index == m_btnProjects) {
        m_projectDock->setVisible(!m_projectDock->isVisible());
        if (m_projectDock->isVisible()) m_projectDock->raise();
    } else if (index == m_btnGit) {
        if (m_actGit) m_actGit->toggle();
    } else if (index == m_btnHelp) {
        showAboutDialog();
    }
}

void MainWindow::updateModeBarKit()
{
    if (!m_modeBar || !m_buildConfigs) return;
    QString name = m_currentProjectName;
    if (name.isEmpty() && !m_currentProjectPath.isEmpty())
        name = QFileInfo(m_currentProjectPath).fileName();
    m_modeBar->setKit(name.isEmpty() ? tr("No project") : name,
                      m_buildConfigs->active().name);
}

// Clicking the project indicator: pick the active configuration, edit them,
// or open another project — as Qt Creator's kit selector does.
void MainWindow::showKitMenu()
{
    QMenu menu(this);
    populateBuildConfigMenu(&menu);
    menu.addSeparator();
    menu.addAction(tr("Open Project Folder…"), this, &MainWindow::onOpenProjectFolder);
    menu.exec(m_modeBar->kitGlobalRect().topRight());
}

// ─────────────────────────────────────────────────────────────────────────────
// Build configurations
// ─────────────────────────────────────────────────────────────────────────────

// Configurations belong to the folder Build/Run work in: the open project,
// or the current file's folder when no project is open.
void MainWindow::ensureBuildConfigs()
{
    const QString dir  = buildWorkingDir();
    const QString name = m_currentProjectName.isEmpty() ? QFileInfo(dir).fileName()
                                                        : m_currentProjectName;
    if (dir != m_buildConfigs->projectDir() || name != m_buildConfigs->projectName())
        m_buildConfigs->setProject(dir, name);
    const EditorTabWidget *tab = currentEditorTab();
    m_buildConfigs->setCurrentFile(tab ? tab->filePath() : QString());
}

void MainWindow::populateBuildConfigMenu(QMenu *menu)
{
    ensureBuildConfigs();
    menu->clear();
    auto *group = new QActionGroup(menu);
    const auto &configs = m_buildConfigs->configurations();
    for (int i = 0; i < configs.size(); ++i) {
        QAction *a = menu->addAction(configs[i].name);
        a->setCheckable(true);
        a->setChecked(i == m_buildConfigs->activeIndex());
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, i] {
            QString error;
            if (!m_buildConfigs->setActiveIndex(i, &error))
                statusBar()->showMessage(error, 6000);
            else
                statusBar()->showMessage(tr("Build configuration: %1")
                                             .arg(m_buildConfigs->active().name), 3000);
        });
    }
    menu->addSeparator();
    menu->addAction(tr("Edit Build Configurations…"), this, &MainWindow::editBuildConfigurations);
}

void MainWindow::editBuildConfigurations()
{
    ensureBuildConfigs();
    BuildConfigDialog dlg(*m_buildConfigs, this);
    if (dlg.exec() != QDialog::Accepted) return;
    QString error;
    if (!m_buildConfigs->setConfigurations(dlg.configurations(), dlg.activeIndex(), &error))
        QMessageBox::warning(this, tr("Build Configurations"),
                             tr("The changes apply to this session but could not be saved.\n%1")
                                 .arg(error));
}

// The active configuration's command for `field`, with variables expanded;
// empty (and a status message) if the configuration has none.
QString MainWindow::configCommand(const QString BuildConfiguration::*field, const QString &what)
{
    ensureBuildConfigs();
    const BuildConfiguration cfg = m_buildConfigs->active();
    if ((cfg.*field).contains(QLatin1String("${File")) && m_buildConfigs->currentFile().isEmpty()) {
        statusBar()->showMessage(tr("Open the file to %1 first — the \"%2\" configuration "
                                    "works on the file in the editor.").arg(what, cfg.name), 6000);
        return {};
    }
    const QString command = m_buildConfigs->expand(cfg.*field, cfg).trimmed();
    if (command.isEmpty())
        statusBar()->showMessage(tr("The \"%1\" configuration has no %2 command — "
                                    "set one in Project ▸ Build Configuration ▸ Edit.")
                                     .arg(cfg.name, what), 6000);
    return command;
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

void MainWindow::showWelcomeTab()
{
    if (m_welcomeTab) {
        int idx = ui->tabWidgetEditor->indexOf(m_welcomeTab);
        if (idx >= 0) {
            ui->tabWidgetEditor->setCurrentIndex(idx);
            return;
        }
        m_welcomeTab = nullptr; // was closed – fall through and recreate
    }

    m_welcomeTab = new WelcomeTabWidget(ui->tabWidgetEditor);
    connect(m_welcomeTab, &WelcomeTabWidget::newFileRequested,
            this, &MainWindow::on_actionNewFile_triggered);
    connect(m_welcomeTab, &WelcomeTabWidget::openFileRequested,
            this, &MainWindow::handleOpenFileTriggered);
    connect(m_welcomeTab, &WelcomeTabWidget::newProjectRequested,
            this, &MainWindow::on_actionNewProject_triggered);
    connect(m_welcomeTab, &WelcomeTabWidget::openProjectRequested,
            this, &MainWindow::onOpenProjectFolder);

    int index = ui->tabWidgetEditor->addTab(m_welcomeTab, tr("Welcome"));
    ui->tabWidgetEditor->setCurrentIndex(index);
}

// Open (or switch to) a file and put the cursor on a 1-based line/column.
void MainWindow::openFileAtLine(const QString &file, int line, int column)
{
    if (!QFileInfo::exists(file)) return;
    EditorTabWidget *tab = nullptr;
    for (EditorTabWidget *t : std::as_const(openEditorTabs))
        if (QFileInfo(t->filePath()) == QFileInfo(file)) { tab = t; break; }
    if (tab) ui->tabWidgetEditor->setCurrentWidget(tab);
    else     createNewTab(file);   // also focuses an already-open large file

    if (auto *large = qobject_cast<LargeFileTab *>(ui->tabWidgetEditor->currentWidget())) {
        large->view()->goToLine(qMax(1, line));
        large->view()->setFocus();
    } else if (auto *t = currentEditorTab()) {
        QPlainTextEdit *ed = t->getPlainTextEdit();
        QTextCursor c(ed->document()->findBlockByNumber(qMax(0, line - 1)));
        c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qMax(0, column - 1));
        ed->setTextCursor(c);
        ed->centerCursor();
        ed->setFocus();
    }
}

// Startup path from the command line: a folder becomes the project, a file
// opens in a tab.
void MainWindow::openPath(const QString &path)
{
    const QFileInfo fi(path);
    if (fi.isDir())       setCurrentProjectPath(fi.absoluteFilePath());
    else if (fi.isFile()) createNewTab(fi.absoluteFilePath());
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

    // Huge files (≥ 32 MB, e.g. 10M+ lines) open in the memory-mapped,
    // read-only viewer: QPlainTextEdit would need GBs of RAM for them.
    if (!filePath.isEmpty() && QFileInfo(filePath).size() >= LargeFileTab::kThresholdBytes) {
        for (int i = 0; i < ui->tabWidgetEditor->count(); ++i) {
            auto *lt = qobject_cast<LargeFileTab *>(ui->tabWidgetEditor->widget(i));
            if (lt && QFileInfo(lt->filePath()) == QFileInfo(filePath)) {
                ui->tabWidgetEditor->setCurrentIndex(i);
                return;
            }
        }
        auto *large = new LargeFileTab(ui->tabWidgetEditor);
        QString error;
        if (!large->open(filePath, &error)) {
            large->deleteLater();
            QMessageBox::warning(this, tr("File Open Error"),
                                 tr("Could not open file: %1\n%2").arg(filePath, error));
            return;
        }
        const int index = ui->tabWidgetEditor->addTab(large, QFileInfo(filePath).fileName());
        ui->tabWidgetEditor->setTabToolTip(index, tr("%1 (large file mode)").arg(filePath));
        connect(large, &LargeFileTab::modificationChanged, this, [this, large](bool modified) {
            const int i = ui->tabWidgetEditor->indexOf(large);
            if (i >= 0)
                ui->tabWidgetEditor->setTabText(i, QFileInfo(large->filePath()).fileName() +
                                                       (modified ? QStringLiteral("*") : QString()));
        });
        ui->tabWidgetEditor->setCurrentIndex(index);
        return;
    }

    EditorTabWidget *editorTab = new EditorTabWidget(filePath, ui->tabWidgetEditor);
    openEditorTabs.append(editorTab);

    connect(editorTab, &EditorTabWidget::modificationChanged,
            this, &MainWindow::updateTabTitle);
    connect(editorTab, &EditorTabWidget::titleChanged,
            this, &MainWindow::updateTabTitleOnRename);
    connect(editorTab, &EditorTabWidget::openFileRequested,
            this, [this](const QString &path) { createNewTab(path); });

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
    applyDiagnostics(editorTab);
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
        if (!maybeSaveLargeTab(qobject_cast<LargeFileTab *>(ui->tabWidgetEditor->widget(i)))) {
            event->ignore();
            return;
        }
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

    if (m_assistant && m_assistant->hasConversation()) {
        m_assistant->saveSession();
        std::cout << "\nKayte Assistant session saved. Resume it with:\n  KayteIDE --resume "
                  << m_assistant->sessionId().toStdString() << "\n" << std::flush;
    }

    qDebug() << "=== closeEvent ACCEPTING - closing application ===";
    event->accept();
}

void MainWindow::on_actionNewFile_triggered() { createNewTab(); }

// Ask to save a modified large file. Returns false if the user cancels.
bool MainWindow::maybeSaveLargeTab(LargeFileTab *large)
{
    if (!large || !large->isModified()) return true;
    ui->tabWidgetEditor->setCurrentWidget(large);
    const auto choice = QMessageBox::warning(
        this, tr("Unsaved Changes"),
        tr("Save changes to %1?").arg(QFileInfo(large->filePath()).fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Cancel) return false;
    if (choice == QMessageBox::Save && !large->saveAndWait()) {
        QMessageBox::critical(this, tr("Save Failed"),
                              tr("Could not save %1.").arg(large->filePath()));
        return false;
    }
    return true;
}

void MainWindow::handleSaveFileTriggered()
{
    if (auto *large = qobject_cast<LargeFileTab *>(ui->tabWidgetEditor->currentWidget())) {
        large->save();                               // background; progress in its info bar
        return;
    }
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

    const QString modeString =
        currentDevelopmentMode == DevelopmentMode::RAD ? QStringLiteral("RAD") : QStringLiteral("Editor");
    xml.writeTextElement("DevelopmentMode", modeString);

    // The active build configuration (all of them live in .kayteide/build.json).
    ensureBuildConfigs();
    const BuildConfiguration cfg = m_buildConfigs->active();
    xml.writeStartElement("BuildSettings");
    xml.writeTextElement("Configuration", cfg.name);
    xml.writeTextElement("BuildCommand", cfg.build);
    xml.writeTextElement("RunCommand",   cfg.run);
    xml.writeTextElement("CleanCommand", cfg.clean);
    xml.writeTextElement("DebugCommand", cfg.debug);
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
        tr("Text Files (*.txt *.vb *.cpp *.h *.kayte *.kyt *.pas *.pp *.dpr);;Markdown (*.md *.markdown);;All Files (*.*)"));
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

    QWidget *widget = ui->tabWidgetEditor->widget(index);
    EditorTabWidget *tab = qobject_cast<EditorTabWidget*>(widget);
    if (!tab) {
        // Non-editor tab (Welcome page, large file): ask to save if needed, close.
        if (!maybeSaveLargeTab(qobject_cast<LargeFileTab *>(widget)))
            return;
        if (widget == m_welcomeTab)
            m_welcomeTab = nullptr;
        ui->tabWidgetEditor->removeTab(index);
        widget->deleteLater();
        if (ui->tabWidgetEditor->count() == 0)
            createNewTab();
        return;
    }

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

QString MainWindow::buildWorkingDir() const
{
    if (!m_currentProjectPath.isEmpty())
        return m_currentProjectPath;
    if (auto *t = currentEditorTab(); t && !t->filePath().isEmpty())
        return QFileInfo(t->filePath()).absolutePath();
    return QDir::currentPath();
}

// Build/Run/Clean/Debug execute a shell command inside the project folder
// (and `make` runs that folder's Makefile), so code from a folder you just
// downloaded would run with your privileges. Like VS Code's workspace trust,
// ask once per folder + exact command; a changed command asks again.
bool MainWindow::confirmTrustedRun(const QString &command, const QString &dir)
{
    const QString folder = QFileInfo(dir).canonicalFilePath().isEmpty()
                               ? dir : QFileInfo(dir).canonicalFilePath();
    const QByteArray digest = QCryptographicHash::hash(
        (folder + QLatin1Char('\n') + command).toUtf8(), QCryptographicHash::Sha256).toHex();

    QSettings settings;
    QStringList trusted = settings.value(QStringLiteral("security/trustedRuns")).toStringList();
    if (trusted.contains(QString::fromLatin1(digest))) return true;

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Trust this project folder?"));
    box.setText(tr("KayteIDE is about to run this command in:<br><code>%1</code>")
                    .arg(folder.toHtmlEscaped()));
    box.setInformativeText(tr("<pre>%1</pre>Only continue if you trust the folder's contents "
                              "(Makefiles, scripts and build files run with your permissions).")
                               .arg(command.toHtmlEscaped()));
    QPushButton *trust = box.addButton(tr("Trust and Run"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() != trust) return false;

    trusted << QString::fromLatin1(digest);
    settings.setValue(QStringLiteral("security/trustedRuns"), trusted);
    return true;
}

void MainWindow::buildProject()
{
    m_afterBuild = AfterBuild::Nothing;
    const QString command = configCommand(&BuildConfiguration::build, tr("build"));
    if (command.isEmpty() || !confirmTrustedRun(command, buildWorkingDir())) return;
    m_terminalDock->show();
    m_bottomPanel->runTask(tr("Build"), command, buildWorkingDir());
}

void MainWindow::runProject()
{
    const QString command = configCommand(&BuildConfiguration::run, tr("run"));
    if (command.isEmpty() || !confirmTrustedRun(command, buildWorkingDir())) return;
    if (buildFirst(AfterBuild::Run, command)) return;
    m_terminalDock->show();
    m_bottomPanel->runTask(tr("Run"), command, buildWorkingDir());
}

// "Build before running": start the build and run `thenCommand` from
// onTaskFinished if it succeeds. Returns false when no build is needed.
bool MainWindow::buildFirst(AfterBuild then, const QString &thenCommand)
{
    const BuildConfiguration cfg = m_buildConfigs->active();
    if (!cfg.buildBeforeRun) return false;
    const QString build = m_buildConfigs->expand(cfg.build, cfg).trimmed();
    if (build.isEmpty() || !confirmTrustedRun(build, buildWorkingDir())) return false;

    // Build output goes to the files being edited, so save them first.
    for (EditorTabWidget *tab : std::as_const(openEditorTabs))
        if (tab->isModified() && !tab->filePath().isEmpty())
            tab->saveFile(tab->filePath());

    m_afterBuild = then;
    m_afterBuildCommand = thenCommand;
    m_terminalDock->show();
    statusBar()->showMessage(tr("Building \"%1\"…").arg(cfg.name));
    m_bottomPanel->runTask(tr("Build"), build, buildWorkingDir());
    return true;
}

void MainWindow::onTaskFinished(int exitCode, bool crashed)
{
    const AfterBuild then = std::exchange(m_afterBuild, AfterBuild::Nothing);
    const QString command = std::exchange(m_afterBuildCommand, QString());
    if (then == AfterBuild::Nothing) return;

    if (exitCode != 0 || crashed) {
        int errors = 0;
        for (const auto &list : std::as_const(m_diagnostics)) errors += list.size();
        statusBar()->showMessage(errors > 0
            ? tr("Build failed with %n problem(s) — marked in the editor.", nullptr, errors)
            : tr("Build failed — see the Build output."), 8000);
        m_bottomPanel->showView(errors > 0 ? BottomPanel::Problems : BottomPanel::Output);
        return;
    }
    statusBar()->clearMessage();
    if (then == AfterBuild::Run)
        m_bottomPanel->runTask(tr("Run"), command, buildWorkingDir());
    else
        m_bottomPanel->startDebugger(command, buildWorkingDir());
}

// ─────────────────────────────────────────────────────────────────────────────
// Toolchain installer
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::maybeRunFirstSetup()
{
    if (ToolchainSetupDialog::installerScript().isEmpty() || ToolchainSetupDialog::isInstalled())
        return;
    if (QSettings().value(QStringLiteral("setup/declined")).toBool()) {
        statusBar()->showMessage(tr("The toolchain is not installed — "
                                    "Tools ▸ Install / Update Toolchain… installs it."), 10000);
        return;
    }
    showToolchainSetup(true);
}

void MainWindow::showToolchainSetup(bool firstRun)
{
    if (m_setupDialog) {   // already installing: bring it back
        m_setupDialog->show();
        m_setupDialog->raise();
        m_setupDialog->activateWindow();
        return;
    }
    m_setupDialog = new ToolchainSetupDialog(this);
    m_setupDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_setupDialog, &ToolchainSetupDialog::finished, this,
            [this, firstRun](bool ok, bool cancelled) {
        QSettings settings;
        if (ok) {
            settings.remove(QStringLiteral("setup/declined"));
            // Build configurations detected before the tools existed fall back
            // to tools on PATH; detect them again.
            m_buildConfigs->setProject(m_buildConfigs->projectDir(), m_buildConfigs->projectName());
            statusBar()->showMessage(tr("The KayteIDE toolchain is installed."), 8000);
        } else if (cancelled && firstRun) {
            // Don't start it on every launch; the Tools menu still offers it.
            settings.setValue(QStringLiteral("setup/declined"), true);
        }
    });
    m_setupDialog->show();
    m_setupDialog->start();
}

QString MainWindow::diagnosticKey(const QString &file)
{
    const QFileInfo fi(file);
    const QString canonical = fi.canonicalFilePath();
    return canonical.isEmpty() ? QDir::cleanPath(fi.absoluteFilePath()) : canonical;
}

void MainWindow::applyDiagnostics(EditorTabWidget *tab)
{
    if (!tab || tab->filePath().isEmpty()) return;
    tab->setDiagnostics(m_diagnostics.value(diagnosticKey(tab->filePath())));
}

void MainWindow::cleanProject()
{
    m_afterBuild = AfterBuild::Nothing;
    const QString command = configCommand(&BuildConfiguration::clean, tr("clean"));
    if (command.isEmpty() || !confirmTrustedRun(command, buildWorkingDir())) return;
    m_terminalDock->show();
    m_bottomPanel->runTask(tr("Build"), command, buildWorkingDir());
}

void MainWindow::debugProject()
{
    const QString command = configCommand(&BuildConfiguration::debug, tr("debug"));
    if (command.isEmpty() || !confirmTrustedRun(command, buildWorkingDir())) return;
    if (buildFirst(AfterBuild::Debug, command)) return;
    m_terminalDock->show();
    m_bottomPanel->startDebugger(command, buildWorkingDir());
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

void MainWindow::updateLineNumberAreaWidth(int newBlockCount)
{
    // Delegate to the active tab's line number gutter.
    if (EditorTabWidget *tab = currentEditorTab())
        if (LineNumberArea *lna = tab->getLineNumberArea())
            lna->updateWidth(newBlockCount);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    // Each tab's gutter lives in the tab's layout and resizes with it.
}

void MainWindow::updateLineNumberArea(const QRect &rect, int dy)
{
    // Forwarded from EditorTabWidget's updateRequest signal.
    // The LineNumberArea's own onUpdateRequest slot handles this;
    // this method exists for compatibility with the header declaration.
    Q_UNUSED(rect) Q_UNUSED(dy)
}

void MainWindow::populateProjectList()
{
    ui->projectListWidget->clear();
    ui->projectListWidget->addItem(tr("My First RAD Project"));
    ui->projectListWidget->addItem(tr("Sample Game Engine"));
    ui->projectListWidget->addItem(tr("Business Application Prototype"));
    ui->projectListWidget->addItem(tr("Another Cool Project"));
}