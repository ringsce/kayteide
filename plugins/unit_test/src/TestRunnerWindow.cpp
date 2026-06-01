#include "TestRunnerWindow.h"
#include "MultiTestProcess.h"
#include "TestProcess.h"
#include "LanguageRunner.h"
#include "SyntaxHighlighter.h"

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QSplitter>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTabWidget>
#include <QHeaderView>
#include <QStatusBar>
#include <QDateTime>
#include <QFont>
#include <QColor>
#include <QFileInfo>
#include <QTextStream>
#include <QFile>
#include <QFrame>

// ── Helpers ───────────────────────────────────────────────────────────────────

QString TestRunnerWindow::languageFromPath(const QString &path)
{
    return LanguageRunner::fromFile(path).language;
}

SyntaxHighlighter::Language TestRunnerWindow::highlighterLang(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "c" || ext == "cpp" || ext == "cxx" || ext == "cc" ||
        ext == "h" || ext == "hpp" || ext == "hxx")
        return SyntaxHighlighter::C_CPP;
    if (ext == "php")   return SyntaxHighlighter::PHP;
    if (ext == "pl")    return SyntaxHighlighter::Perl;
    if (ext == "pas")   return SyntaxHighlighter::Pascal;
    if (ext == "kayte") return SyntaxHighlighter::Kayte;
    return SyntaxHighlighter::Generic;
}

// ── Constructor ───────────────────────────────────────────────────────────────

TestRunnerWindow::TestRunnerWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_multiProc(new MultiTestProcess(this))
    , m_qtProc(new TestProcess(this))
{
    setWindowTitle("Qt6 Multi-Language Test Runner");
    setMinimumSize(1000, 660);
    resize(1280, 760);

    buildUi();
    applyStyle();

    // Multi-language process
    connect(m_multiProc, &MultiTestProcess::started,     this, &TestRunnerWindow::onStarted);
    connect(m_multiProc, &MultiTestProcess::resultReady, this, &TestRunnerWindow::onResult);
    connect(m_multiProc, &MultiTestProcess::logLine,     this, &TestRunnerWindow::onLogLine);
    connect(m_multiProc, &MultiTestProcess::finished,    this, &TestRunnerWindow::onFinished);

    // QTest process
    connect(m_qtProc, &TestProcess::resultReady, this, &TestRunnerWindow::onResult);
    connect(m_qtProc, &TestProcess::logLine,     [this](const QString &l){ onLogLine(l.toHtmlEscaped()); });
    connect(m_qtProc, &TestProcess::finished,    this, &TestRunnerWindow::onFinished);
}

void TestRunnerWindow::setBinaryPath(const QString &path)
{
    auto *item = new QListWidgetItem(QFileInfo(path).fileName());
    item->setData(Qt::UserRole, path);
    item->setToolTip(path);
    m_fileList->addItem(item);
}

// ── UI construction ───────────────────────────────────────────────────────────

void TestRunnerWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QHBoxLayout(central);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    // ════════════════════════════════════════════════════════════════════════
    // LEFT PANEL — file list + controls
    // ════════════════════════════════════════════════════════════════════════
    auto *leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(260);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(6);
    leftLayout->setContentsMargins(10, 10, 6, 10);

    auto *panelTitle = new QLabel("TEST FILES", leftPanel);
    panelTitle->setObjectName("panelTitle");

    m_fileList = new QListWidget(leftPanel);
    m_fileList->setObjectName("fileList");
    m_fileList->setDragDropMode(QAbstractItemView::InternalMove);
    m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    auto *addRemoveRow = new QHBoxLayout;
    m_addBtn    = new QPushButton("＋ Add", leftPanel);
    m_removeBtn = new QPushButton("－ Remove", leftPanel);
    m_addBtn->setObjectName("addBtn");
    m_removeBtn->setObjectName("removeBtn");
    addRemoveRow->addWidget(m_addBtn);
    addRemoveRow->addWidget(m_removeBtn);

    auto *sep = new QFrame(leftPanel);
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("separator");

    m_runBtn  = new QPushButton("▶  Run All", leftPanel);
    m_stopBtn = new QPushButton("■  Stop",    leftPanel);
    m_runBtn->setObjectName("runBtn");
    m_stopBtn->setObjectName("stopBtn");
    m_stopBtn->setEnabled(false);

    // Filter
    auto *filterLbl = new QLabel("Filter results:", leftPanel);
    filterLbl->setObjectName("sectionLabel");
    m_filterEdit = new QLineEdit(leftPanel);
    m_filterEdit->setPlaceholderText("Search…");

    leftLayout->addWidget(panelTitle);
    leftLayout->addWidget(m_fileList, 1);
    leftLayout->addLayout(addRemoveRow);
    leftLayout->addWidget(sep);
    leftLayout->addWidget(m_runBtn);
    leftLayout->addWidget(m_stopBtn);
    leftLayout->addSpacing(4);
    leftLayout->addWidget(filterLbl);
    leftLayout->addWidget(m_filterEdit);

    // ════════════════════════════════════════════════════════════════════════
    // RIGHT PANEL — tabs (Results / Source / Log) + summary
    // ════════════════════════════════════════════════════════════════════════
    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(4);
    rightLayout->setContentsMargins(4, 10, 10, 10);

    // Progress + summary row
    m_progress   = new QProgressBar(rightPanel);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(5);
    m_progress->setRange(0, 1);
    m_progress->setValue(0);

    m_summaryLbl = new QLabel("No tests run yet.", rightPanel);
    m_summaryLbl->setObjectName("summaryLabel");

    // Tabs
    m_tabs = new QTabWidget(rightPanel);
    m_tabs->setObjectName("mainTabs");

    // — Results tab —
    m_tree = new QTreeWidget(m_tabs);
    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({"File / Test", "Lang", "Result", "Message"});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_tree->header()->resizeSection(1, 80);
    m_tree->header()->resizeSection(2, 70);
    m_tree->setAlternatingRowColors(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setAnimated(true);
    m_tabs->addTab(m_tree, "📊  Results");

    // — Source tab —
    m_srcView = new QPlainTextEdit(m_tabs);
    m_srcView->setObjectName("srcView");
    m_srcView->setReadOnly(true);
    m_srcView->setPlaceholderText("Select a file from the list to view its source…");
    m_srcView->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_highlighter = new SyntaxHighlighter(m_srcView->document());
    m_tabs->addTab(m_srcView, "📄  Source");

    // — Log tab —
    m_logView = new QTextEdit(m_tabs);
    m_logView->setObjectName("logView");
    m_logView->setReadOnly(true);
    m_logView->setPlaceholderText("Raw output appears here during test runs…");
    m_tabs->addTab(m_logView, "📋  Log");

    rightLayout->addWidget(m_progress);
    rightLayout->addWidget(m_summaryLbl);
    rightLayout->addWidget(m_tabs, 1);

    root->addWidget(leftPanel);
    root->addWidget(rightPanel, 1);

    statusBar()->showMessage("Ready — add files and press Run All.");

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_addBtn,    &QPushButton::clicked, this, &TestRunnerWindow::onAddFile);
    connect(m_removeBtn, &QPushButton::clicked, this, &TestRunnerWindow::onRemoveFile);
    connect(m_runBtn,    &QPushButton::clicked, this, &TestRunnerWindow::onRun);
    connect(m_stopBtn,   &QPushButton::clicked, this, &TestRunnerWindow::onStop);
    connect(m_tree, &QTreeWidget::itemClicked,  this, &TestRunnerWindow::onTreeItemClicked);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &TestRunnerWindow::onFilterChanged);
    connect(m_fileList, &QListWidget::currentRowChanged, this, &TestRunnerWindow::onFileListChanged);
    connect(m_tabs, &QTabWidget::currentChanged, this, &TestRunnerWindow::onTabChanged);
}

void TestRunnerWindow::applyStyle()
{
    qApp->setStyle("Fusion");
    const QString css = R"(
        * { box-sizing: border-box; }

        QMainWindow, QWidget {
            background-color: #1e1e2e;
            color: #cdd6f4;
            font-family: "JetBrains Mono", "Fira Code", "Consolas", monospace;
            font-size: 13px;
        }

        QWidget#leftPanel {
            background-color: #181825;
            border-right: 1px solid #313244;
        }

        QLabel#panelTitle {
            color: #45475a;
            font-size: 10px;
            letter-spacing: 2px;
            padding: 4px 0 8px 2px;
        }

        QLabel#sectionLabel { color: #6c7086; font-size: 11px; }
        QLabel#summaryLabel { color: #cdd6f4; font-size: 12px; padding: 1px 2px; }

        QFrame#separator { color: #313244; }

        QListWidget#fileList {
            background-color: #11111b;
            border: 1px solid #313244;
            border-radius: 4px;
        }
        QListWidget#fileList::item {
            padding: 6px 8px;
            border-bottom: 1px solid #1e1e2e;
        }
        QListWidget#fileList::item:selected {
            background-color: #313244;
            color: #89b4fa;
        }
        QListWidget#fileList::item:hover { background-color: #1e1e2e; }

        QLineEdit {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 4px 8px;
            color: #cdd6f4;
        }
        QLineEdit:focus { border-color: #89b4fa; }

        QPushButton {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 4px;
            padding: 6px 12px;
            color: #cdd6f4;
        }
        QPushButton:hover   { background-color: #45475a; }
        QPushButton:pressed { background-color: #1e1e2e; }
        QPushButton:disabled { color: #45475a; border-color: #313244; }

        QPushButton#runBtn {
            background-color: #a6e3a1; color: #1e1e2e;
            font-weight: bold; border: none;
        }
        QPushButton#runBtn:hover { background-color: #94e2d5; }
        QPushButton#runBtn:disabled { background-color: #313244; color: #45475a; }

        QPushButton#stopBtn {
            background-color: #f38ba8; color: #1e1e2e;
            font-weight: bold; border: none;
        }
        QPushButton#stopBtn:hover { background-color: #eba0ac; }
        QPushButton#stopBtn:disabled { background-color: #313244; color: #45475a; }

        QPushButton#addBtn    { background-color: #89dceb; color: #1e1e2e; border: none; font-weight: bold; }
        QPushButton#removeBtn { background-color: #45475a; color: #cdd6f4; border: none; }

        QProgressBar {
            border: none; border-radius: 2px;
            background-color: #313244;
        }
        QProgressBar::chunk { background-color: #89b4fa; border-radius: 2px; }

        QTabWidget#mainTabs::pane {
            border: 1px solid #313244;
            border-radius: 4px;
            background-color: #181825;
        }
        QTabBar::tab {
            background: #1e1e2e;
            border: 1px solid #313244;
            border-bottom: none;
            padding: 6px 16px;
            color: #6c7086;
        }
        QTabBar::tab:selected { background: #181825; color: #cdd6f4; border-color: #45475a; }
        QTabBar::tab:hover    { color: #cdd6f4; }

        QTreeWidget {
            background-color: #181825;
            alternate-background-color: #1e1e2e;
            border: none;
        }
        QTreeWidget::item { padding: 3px 0; }
        QTreeWidget::item:selected { background-color: #313244; color: #cdd6f4; }
        QHeaderView::section {
            background-color: #11111b;
            color: #89b4fa;
            border: none;
            border-right: 1px solid #313244;
            border-bottom: 1px solid #313244;
            padding: 5px 8px;
            font-weight: bold;
        }

        QPlainTextEdit#srcView, QTextEdit#logView {
            background-color: #11111b;
            border: none;
            color: #cdd6f4;
            font-family: "JetBrains Mono", "Fira Code", "Consolas", monospace;
            font-size: 12px;
        }

        QStatusBar { background-color: #11111b; color: #45475a; font-size: 11px; }

        QScrollBar:vertical   { background: #1e1e2e; width: 8px; }
        QScrollBar:horizontal { background: #1e1e2e; height: 8px; }
        QScrollBar::handle    { background: #45475a; border-radius: 4px; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
    )";
    qApp->setStyleSheet(css);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void TestRunnerWindow::onAddFile()
{
    const QStringList exts = LanguageRunner::supportedExtensions();
    QStringList filters;
    for (const QString &e : exts) filters << "*." + e;

    // Also allow binary executables (no extension on Linux/Mac)
    const QString filter = "Supported files (" + filters.join(' ') + " *);;All Files (*)";

    const QStringList paths = QFileDialog::getOpenFileNames(
        this, "Add Test Files", QDir::homePath(), filter);

    for (const QString &p : paths) {
        // Avoid duplicates
        bool dup = false;
        for (int i = 0; i < m_fileList->count(); ++i)
            if (m_fileList->item(i)->data(Qt::UserRole).toString() == p) { dup = true; break; }
        if (dup) continue;

        LanguageRunner lr = LanguageRunner::fromFile(p);
        auto *item = new QListWidgetItem(lr.icon + " " + QFileInfo(p).fileName());
        item->setData(Qt::UserRole, p);
        item->setToolTip(p);
        m_fileList->addItem(item);
    }
}

void TestRunnerWindow::onRemoveFile()
{
    for (auto *item : m_fileList->selectedItems())
        delete item;
}

void TestRunnerWindow::onRun()
{
    if (m_fileList->count() == 0) {
        statusBar()->showMessage("Add at least one file first.");
        return;
    }

    resetResults();

    // Build queue
    m_pendingFiles.clear();
    for (int i = 0; i < m_fileList->count(); ++i)
        m_pendingFiles << m_fileList->item(i)->data(Qt::UserRole).toString();

    m_filesTotal = m_pendingFiles.size();
    m_filesDone  = 0;

    m_runBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_progress->setRange(0, m_filesTotal);
    m_progress->setValue(0);

    // Run first file; subsequent files run in onFinished()
    const QString first = m_pendingFiles.takeFirst();
    LanguageRunner lr = LanguageRunner::fromFile(first);
    if (lr.mode == LanguageRunner::Mode::QtTest)
        m_qtProc->run(first);
    else
        m_multiProc->run(first);

    statusBar()->showMessage(QString("Running %1 file(s)…").arg(m_filesTotal));
}

void TestRunnerWindow::onStop()
{
    m_pendingFiles.clear();
    m_multiProc->kill();
    m_qtProc->kill();
}

void TestRunnerWindow::onStarted(const QString &language)
{
    m_logView->append(QString("<br><span style='color:#89b4fa;font-weight:bold'>"
                              "══ %1 ══</span>").arg(language.toHtmlEscaped()));
    m_tabs->setCurrentIndex(2); // switch to log while running
}

void TestRunnerWindow::onResult(const TestResult &r)
{
    ++m_total;

    const QString suiteName = r.testCaseName.isEmpty()
                              ? QFileInfo(r.file.isEmpty()
                                          ? m_pendingFiles.isEmpty()
                                            ? "Unknown" : m_pendingFiles.first()
                                          : r.file).fileName()
                              : r.testCaseName;

    QTreeWidgetItem *suiteItem = getOrCreateSuiteItem(suiteName);

    auto *item = new QTreeWidgetItem(suiteItem);
    // Fill language column from suite's userData if available
    item->setText(0, r.functionName);
    item->setText(1, suiteItem->data(1, Qt::UserRole).toString());
    item->setText(2, r.result);
    item->setText(3, r.message);
    item->setTextAlignment(2, Qt::AlignCenter);

    if (r.result == "PASS") {
        ++m_passed;
        item->setForeground(2, QColor("#a6e3a1"));
    } else if (r.result == "FAIL") {
        ++m_failed;
        item->setForeground(2, QColor("#f38ba8"));
        item->setForeground(0, QColor("#f38ba8"));
        suiteItem->setExpanded(true);
    } else {
        ++m_skipped;
        item->setForeground(2, QColor("#f9e2af"));
    }

    updateSummary();
}

void TestRunnerWindow::onLogLine(const QString &html)
{
    m_logView->append(QString("<span style='color:#a6adc8'>%1</span>").arg(html));
}

void TestRunnerWindow::onFinished(int exitCode)
{
    ++m_filesDone;
    m_progress->setValue(m_filesDone);

    if (!m_pendingFiles.isEmpty()) {
        const QString next = m_pendingFiles.takeFirst();
        LanguageRunner lr = LanguageRunner::fromFile(next);
        if (lr.mode == LanguageRunner::Mode::QtTest)
            m_qtProc->run(next);
        else
            m_multiProc->run(next);
        return;
    }

    // All done
    m_runBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_tabs->setCurrentIndex(0); // switch to results

    const bool allGood = (m_failed == 0 && m_total > 0);
    m_summaryLbl->setText(allGood
        ? QString("✓ All %1 checks passed across %2 file(s)").arg(m_total).arg(m_filesTotal)
        : QString("✗ %1 failed  ✓ %2 passed  ↷ %3 skipped  — %4 file(s)")
              .arg(m_failed).arg(m_passed).arg(m_skipped).arg(m_filesTotal));
    m_summaryLbl->setStyleSheet(allGood
        ? "color:#a6e3a1; font-weight:bold;"
        : "color:#f38ba8; font-weight:bold;");

    statusBar()->showMessage(QString("Done (exit %1) — %2 passed / %3 failed / %4 skipped")
        .arg(exitCode).arg(m_passed).arg(m_failed).arg(m_skipped));
}

void TestRunnerWindow::onFileListChanged(int /*row*/)
{
    auto *cur = m_fileList->currentItem();
    if (!cur) return;
    const QString path = cur->data(Qt::UserRole).toString();
    if (!path.isEmpty())
        loadSourceFile(path);
}

void TestRunnerWindow::onTreeItemClicked(QTreeWidgetItem *item, int)
{
    if (!item || item->text(3).isEmpty()) return;
    m_logView->append(QString("<br><b style='color:#89b4fa'>%1</b>: "
                              "<span style='color:#f38ba8'>%2</span>")
        .arg(item->text(0).toHtmlEscaped(), item->text(3).toHtmlEscaped()));
}

void TestRunnerWindow::onFilterChanged(const QString &text)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *suite = m_tree->topLevelItem(i);
        bool suiteVisible = false;
        for (int j = 0; j < suite->childCount(); ++j) {
            auto *child = suite->child(j);
            const bool match = text.isEmpty()
                || child->text(0).contains(text, Qt::CaseInsensitive)
                || child->text(3).contains(text, Qt::CaseInsensitive);
            child->setHidden(!match);
            if (match) suiteVisible = true;
        }
        suite->setHidden(!suiteVisible);
    }
}

void TestRunnerWindow::onTabChanged(int index)
{
    // Auto-load source when user clicks the Source tab
    if (index == 1) {
        auto *cur = m_fileList->currentItem();
        if (cur) loadSourceFile(cur->data(Qt::UserRole).toString());
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void TestRunnerWindow::resetResults()
{
    m_tree->clear();
    m_logView->clear();
    m_suiteItems.clear();
    m_total = m_passed = m_failed = m_skipped = 0;
    m_filesDone = m_filesTotal = 0;
    m_summaryLbl->setText("Running…");
    m_summaryLbl->setStyleSheet("color:#89b4fa;");
    m_progress->setRange(0, 1);
    m_progress->setValue(0);
}

void TestRunnerWindow::updateSummary()
{
    m_summaryLbl->setText(
        QString("Total: %1  |  ✓ %2  |  ✗ %3  |  ↷ %4  |  Files: %5/%6")
            .arg(m_total).arg(m_passed).arg(m_failed)
            .arg(m_skipped).arg(m_filesDone).arg(m_filesTotal));
}

void TestRunnerWindow::loadSourceFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_srcView->setPlainText("(could not open file)");
        return;
    }
    m_srcView->setPlainText(QTextStream(&f).readAll());
    m_highlighter->setLanguage(highlighterLang(path));

    const LanguageRunner lr = LanguageRunner::fromFile(path);
    statusBar()->showMessage(
        QString("%1  [%2]  —  %3 lines")
            .arg(QFileInfo(path).fileName(), lr.language)
            .arg(m_srcView->document()->lineCount()));
}

QTreeWidgetItem *TestRunnerWindow::getOrCreateSuiteItem(const QString &name)
{
    if (m_suiteItems.contains(name))
        return m_suiteItems[name];

    auto *item = new QTreeWidgetItem(m_tree);
    item->setText(0, name);

    // Try to derive language from file list
    QString lang;
    LanguageRunner lr;
    for (int i = 0; i < m_fileList->count(); ++i) {
        auto *fi = m_fileList->item(i);
        if (fi->data(Qt::UserRole).toString().contains(name) ||
            QFileInfo(fi->data(Qt::UserRole).toString()).fileName() == name) {
            lr   = LanguageRunner::fromFile(fi->data(Qt::UserRole).toString());
            lang = lr.icon + " " + lr.language;
            break;
        }
    }
    item->setText(1, lang);
    item->setData(1, Qt::UserRole, lr.language);

    QFont f = item->font(0);
    f.setBold(true);
    item->setFont(0, f);
    item->setForeground(0, QColor("#89b4fa"));
    item->setExpanded(false);

    m_suiteItems[name] = item;
    return item;
}
