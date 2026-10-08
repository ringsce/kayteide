#include "bottompanel.h"
#include "mainwindow.h"   // TerminalWidget

#include <QButtonGroup>
#include <QComboBox>
#include <QDir>
#include <QFrame>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSet>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

// Flat, upper-case tab buttons with an underline on the active one.
const char *kPanelStyle = R"(
QToolButton#panelTab {
    border: none;
    border-bottom: 1px solid transparent;
    padding: 6px 2px 4px 2px;
    margin: 0 8px;
    color: %1;
    font-size: 11px;
}
QToolButton#panelTab:hover   { color: palette(text); }
QToolButton#panelTab:checked {
    color: palette(text);
    border-bottom: 1px solid palette(highlight);
}
QToolButton#panelAction {
    border: none;
    padding: 2px;
    border-radius: 3px;
}
QToolButton#panelAction:hover { background: palette(midlight); }
)";

QToolButton *makeAction(QWidget *parent, const QIcon &icon,
                        const QString &text, const QString &tip)
{
    auto *b = new QToolButton(parent);
    b->setObjectName(QStringLiteral("panelAction"));
    b->setAutoRaise(true);
    if (icon.isNull()) b->setText(text);
    else               b->setIcon(icon);
    b->setToolTip(tip);
    b->setFixedSize(22, 22);
    return b;
}

QFont monoFont()
{
    QFont mono(QStringLiteral("Monospace"), 10);
    mono.setStyleHint(QFont::TypeWriter);
    return mono;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

BottomPanel::BottomPanel(QWidget *parent)
    : QWidget(parent)
{
    // Inactive tabs: text colour faded ~40% toward the background, like VS Code.
    const QColor fg = palette().color(QPalette::WindowText);
    const QColor bg = palette().color(QPalette::Window);
    const QColor dim = QColor::fromRgbF(fg.redF()   * 0.6f + bg.redF()   * 0.4f,
                                        fg.greenF() * 0.6f + bg.greenF() * 0.4f,
                                        fg.blueF()  * 0.6f + bg.blueF()  * 0.4f);
    setStyleSheet(QString::fromLatin1(kPanelStyle).arg(dim.name()));

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_stack = new QStackedWidget(this);
    // Order must match the View enum.
    m_stack->addWidget(createProblemsView());
    m_stack->addWidget(createOutputView());
    m_stack->addWidget(createDebugConsoleView());
    m_stack->addWidget(createTerminalView());
    m_stack->addWidget(createPortsView());

    lay->addWidget(createHeader());
    lay->addWidget(m_stack, 1);

    showView(Terminal);
}

BottomPanel::~BottomPanel()
{
    for (QProcess *p : { m_task, m_debugger }) {
        if (p && p->state() != QProcess::NotRunning) {
            p->kill();
            p->waitForFinished(1000);
        }
    }
}

QWidget *BottomPanel::createHeader()
{
    auto *header = new QWidget(this);
    auto *hl     = new QHBoxLayout(header);
    hl->setContentsMargins(4, 0, 6, 0);
    hl->setSpacing(2);

    // ── Tabs ──────────────────────────────────────────────────────────────────
    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);
    const QStringList titles = { tr("PROBLEMS"), tr("OUTPUT"), tr("DEBUG CONSOLE"),
                                 tr("TERMINAL"), tr("PORTS") };
    for (int i = 0; i < titles.size(); ++i) {
        auto *tab = new QToolButton(header);
        tab->setObjectName(QStringLiteral("panelTab"));
        tab->setText(titles.at(i));
        tab->setCheckable(true);
        tab->setCursor(Qt::PointingHandCursor);
        m_tabGroup->addButton(tab, i);
        hl->addWidget(tab);
        if (i == Problems) m_problemsTab = tab;
    }
    connect(m_tabGroup, &QButtonGroup::idClicked, this,
            [this](int id) { showView(static_cast<View>(id)); });

    hl->addStretch(1);

    // ── Per-view actions ──────────────────────────────────────────────────────
    m_outputChannel = new QComboBox(header);
    m_outputChannel->setMinimumWidth(120);
    connect(m_outputChannel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int i) { if (i >= 0) m_outputStack->setCurrentIndex(i); });

    m_terminalPick = new QComboBox(header);
    m_terminalPick->setMinimumWidth(100);
    connect(m_terminalPick, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int i) { if (i >= 0) m_terminals->setCurrentIndex(i); });

    m_btnNewTerm = makeAction(header, QIcon(), QStringLiteral("+"), tr("New Terminal"));
    connect(m_btnNewTerm, &QToolButton::clicked, this, [this] {
        addTerminal();
    });

    m_btnKillTerm = makeAction(header, style()->standardIcon(QStyle::SP_TrashIcon),
                               QString(), tr("Kill Terminal"));
    connect(m_btnKillTerm, &QToolButton::clicked, this, &BottomPanel::killTerminal);

    m_btnClear = makeAction(header, style()->standardIcon(QStyle::SP_DialogResetButton),
                            QString(), tr("Clear"));
    connect(m_btnClear, &QToolButton::clicked, this, [this] {
        switch (currentView()) {
        case Problems:     clearProblems(); break;
        case Output:
            if (auto *v = qobject_cast<QPlainTextEdit *>(m_outputStack->currentWidget()))
                v->clear();
            break;
        case DebugConsole: m_debugView->clear(); break;
        default: break;
        }
    });

    m_btnRefresh = makeAction(header, style()->standardIcon(QStyle::SP_BrowserReload),
                              QString(), tr("Refresh Ports"));
    connect(m_btnRefresh, &QToolButton::clicked, this, &BottomPanel::refreshPorts);

    m_btnMaximize = makeAction(header, style()->standardIcon(QStyle::SP_TitleBarMaxButton),
                               QString(), tr("Maximize Panel Size"));
    m_btnMaximize->setCheckable(true);
    connect(m_btnMaximize, &QToolButton::toggled, this, [this](bool on) {
        m_btnMaximize->setIcon(style()->standardIcon(
            on ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton));
        m_btnMaximize->setToolTip(on ? tr("Restore Panel Size")
                                     : tr("Maximize Panel Size"));
        emit maximizeToggled(on);
    });

    auto *btnClose = makeAction(header, style()->standardIcon(QStyle::SP_TitleBarCloseButton),
                                QString(), tr("Hide Panel"));
    connect(btnClose, &QToolButton::clicked, this, &BottomPanel::closeRequested);

    for (QWidget *w : std::initializer_list<QWidget *>{
             m_outputChannel, m_terminalPick, m_btnNewTerm, m_btnKillTerm,
             m_btnClear, m_btnRefresh })
        hl->addWidget(w);

    auto *sep = new QFrame(header);
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedHeight(16);
    hl->addSpacing(4);
    hl->addWidget(sep);
    hl->addSpacing(4);
    hl->addWidget(m_btnMaximize);
    hl->addWidget(btnClose);

    // First terminal / channel exist before the header is built.
    for (int i = 0; i < m_terminals->count(); ++i)
        m_terminalPick->addItem(QStringLiteral("%1: bash").arg(i + 1));

    return header;
}

void BottomPanel::showView(View view)
{
    m_stack->setCurrentIndex(view);
    if (auto *b = m_tabGroup->button(view)) b->setChecked(true);
    updateHeaderActions();
    if (view == Terminal) {
        if (auto *t = currentTerminal()) t->setFocus();
    } else if (view == Ports && m_ports->rowCount() == 0) {
        refreshPorts();
    }
}

BottomPanel::View BottomPanel::currentView() const
{
    return static_cast<View>(m_stack->currentIndex());
}

void BottomPanel::updateHeaderActions()
{
    const View v = currentView();
    m_outputChannel->setVisible(v == Output);
    m_terminalPick->setVisible(v == Terminal);
    m_btnNewTerm->setVisible(v == Terminal);
    m_btnKillTerm->setVisible(v == Terminal);
    m_btnClear->setVisible(v == Problems || v == Output || v == DebugConsole);
    m_btnRefresh->setVisible(v == Ports);
}

QPlainTextEdit *BottomPanel::makeConsole()
{
    auto *view = new QPlainTextEdit(this);
    view->setReadOnly(true);
    view->setLineWrapMode(QPlainTextEdit::NoWrap);
    view->setFont(monoFont());
    view->setFrameShape(QFrame::NoFrame);
    view->setMaximumBlockCount(20000);
    return view;
}

// ─────────────────────────────────────────────────────────────────────────────
// PROBLEMS
// ─────────────────────────────────────────────────────────────────────────────

QWidget *BottomPanel::createProblemsView()
{
    m_problems = new QTreeWidget(this);
    m_problems->setColumnCount(3);
    m_problems->setHeaderLabels({ tr("Message"), tr("File"), tr("Position") });
    m_problems->setRootIsDecorated(false);
    m_problems->setAlternatingRowColors(true);
    m_problems->setFrameShape(QFrame::NoFrame);
    m_problems->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_problems->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_problems->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_problems->header()->setStretchLastSection(false);
    connect(m_problems, &QTreeWidget::itemDoubleClicked,
            this, &BottomPanel::onProblemDoubleClicked);
    return m_problems;
}

void BottomPanel::clearProblems()
{
    m_problems->clear();
    m_errorCount = m_warningCount = 0;
    updateProblemsBadge();
    emit problemsCleared();
}

void BottomPanel::addProblem(const QString &file, int line, int column,
                             const QString &severity, const QString &message)
{
    const QString sev = severity.toLower();
    QStyle::StandardPixmap icon = QStyle::SP_MessageBoxInformation;
    if (sev.contains(QLatin1String("error")) || sev == QLatin1String("fatal")) {
        icon = QStyle::SP_MessageBoxCritical;
        ++m_errorCount;
    } else if (sev == QLatin1String("warning")) {
        icon = QStyle::SP_MessageBoxWarning;
        ++m_warningCount;
    }

    auto *item = new QTreeWidgetItem(m_problems);
    item->setIcon(0, style()->standardIcon(icon));
    item->setText(0, message);
    item->setText(1, QFileInfo(file).fileName());
    item->setToolTip(1, file);
    item->setText(2, column > 0 ? QStringLiteral("Ln %1, Col %2").arg(line).arg(column)
                                : QStringLiteral("Ln %1").arg(line));
    item->setData(0, Qt::UserRole,     file);
    item->setData(0, Qt::UserRole + 1, line);
    item->setData(0, Qt::UserRole + 2, column);
    updateProblemsBadge();
    emit problemAdded(file, line, column, severity, message);
}

void BottomPanel::updateProblemsBadge()
{
    const int total = m_errorCount + m_warningCount;
    m_problemsTab->setText(total > 0 ? tr("PROBLEMS  %1").arg(total)
                                     : tr("PROBLEMS"));
    m_problemsTab->setToolTip(tr("%1 error(s), %2 warning(s)")
                                  .arg(m_errorCount).arg(m_warningCount));
}

void BottomPanel::onProblemDoubleClicked(QTreeWidgetItem *item, int)
{
    emit problemActivated(item->data(0, Qt::UserRole).toString(),
                          item->data(0, Qt::UserRole + 1).toInt(),
                          item->data(0, Qt::UserRole + 2).toInt());
}

// Recognises GCC/Clang ("file:12:5: error: msg") and Free Pascal / Delphi
// ("file.pas(12,5) Error: msg") diagnostics.
void BottomPanel::parseDiagnostics(const QString &text)
{
    static const QRegularExpression gcc(
        QStringLiteral(R"(^(.+?):(\d+):(?:(\d+):)?\s*(fatal error|error|warning|note):\s*(.*)$)"));
    static const QRegularExpression fpc(
        QStringLiteral(R"(^(.+?)\((\d+)(?:,(\d+))?\)\s+(Fatal|Error|Warning|Note|Hint):\s*(.*)$)"));

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        auto m = gcc.match(line);
        if (!m.hasMatch()) m = fpc.match(line);
        if (!m.hasMatch()) continue;

        QString file = m.captured(1);
        if (QFileInfo(file).isRelative() && !m_taskWorkingDir.isEmpty())
            file = QDir(m_taskWorkingDir).absoluteFilePath(file);
        addProblem(QDir::cleanPath(file), m.captured(2).toInt(),
                   m.captured(3).toInt(), m.captured(4), m.captured(5));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// OUTPUT
// ─────────────────────────────────────────────────────────────────────────────

QWidget *BottomPanel::createOutputView()
{
    m_outputStack = new QStackedWidget(this);
    return m_outputStack;
}

void BottomPanel::appendOutput(const QString &channel, const QString &text)
{
    int idx = m_outputChannel->findText(channel);
    if (idx < 0) {
        m_outputStack->addWidget(makeConsole());
        m_outputChannel->addItem(channel);
        idx = m_outputChannel->count() - 1;
    }
    auto *view = static_cast<QPlainTextEdit *>(m_outputStack->widget(idx));
    QTextCursor c(view->document());
    c.movePosition(QTextCursor::End);
    c.insertText(text);
    view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());
}

void BottomPanel::runTask(const QString &channel, const QString &command,
                          const QString &workingDir)
{
    if (m_task && m_task->state() != QProcess::NotRunning) {
        appendOutput(m_taskChannel, tr("\n* Previous task was terminated.\n"));
        m_task->disconnect(this);
        m_task->kill();
        m_task->waitForFinished(1000);
        m_task->deleteLater();
        m_task = nullptr;
    }

    m_taskChannel    = channel;
    m_taskWorkingDir = workingDir;
    m_taskPending.clear();
    clearProblems();

    appendOutput(channel, tr("> Executing task: %1 <\n\n").arg(command));
    m_outputChannel->setCurrentIndex(m_outputChannel->findText(channel));
    showView(Output);

    m_task = new QProcess(this);
    m_task->setProcessChannelMode(QProcess::MergedChannels);
    if (!workingDir.isEmpty()) m_task->setWorkingDirectory(workingDir);
    connect(m_task, &QProcess::readyReadStandardOutput, this, &BottomPanel::onTaskOutput);
    connect(m_task, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BottomPanel::onTaskFinished);
    connect(m_task, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            appendOutput(m_taskChannel, tr("* Failed to start task.\n"));
            emit taskFinished(-1, true);
        }
    });

#ifdef Q_OS_WIN
    m_task->start(QStringLiteral("cmd.exe"), { QStringLiteral("/c"), command });
#else
    m_task->start(QStringLiteral("/bin/sh"), { QStringLiteral("-c"), command });
#endif
}

void BottomPanel::onTaskOutput()
{
    const QString text = QString::fromLocal8Bit(m_task->readAllStandardOutput());
    appendOutput(m_taskChannel, text);

    // Only parse complete lines; keep the tail for the next chunk.
    m_taskPending += text;
    const int nl = m_taskPending.lastIndexOf(QLatin1Char('\n'));
    if (nl >= 0) {
        parseDiagnostics(m_taskPending.left(nl));
        m_taskPending.remove(0, nl + 1);
    }
}

void BottomPanel::onTaskFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_taskPending.isEmpty()) {
        parseDiagnostics(m_taskPending);
        m_taskPending.clear();
    }
    appendOutput(m_taskChannel,
                 status == QProcess::CrashExit
                     ? tr("\n* The task crashed.\n\n")
                     : tr("\n* The task finished with exit code %1.\n\n").arg(exitCode));
    emit taskFinished(exitCode, status == QProcess::CrashExit);
}

// ─────────────────────────────────────────────────────────────────────────────
// DEBUG CONSOLE
// ─────────────────────────────────────────────────────────────────────────────

QWidget *BottomPanel::createDebugConsoleView()
{
    auto *w  = new QWidget(this);
    auto *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    m_debugView = makeConsole();
    m_debugView->setPlaceholderText(
        tr("Start a debug session (Run ▸ Debug) to evaluate expressions here."));

    m_debugInput = new QLineEdit(w);
    m_debugInput->setFont(monoFont());
    m_debugInput->setPlaceholderText(QStringLiteral("❯"));
    m_debugInput->setFrame(false);
    connect(m_debugInput, &QLineEdit::returnPressed, this, &BottomPanel::onDebugInput);

    auto *sep = new QFrame(w);
    sep->setFrameShape(QFrame::HLine);

    vl->addWidget(m_debugView, 1);
    vl->addWidget(sep);
    vl->addWidget(m_debugInput);
    return w;
}

void BottomPanel::startDebugger(const QString &command, const QString &workingDir)
{
    if (m_debugger && m_debugger->state() != QProcess::NotRunning) {
        m_debugger->disconnect(this);
        m_debugger->kill();
        m_debugger->waitForFinished(1000);
        m_debugger->deleteLater();
    }

    m_debugView->appendPlainText(tr("── Debug session: %1 ──").arg(command));
    showView(DebugConsole);

    m_debugger = new QProcess(this);
    m_debugger->setProcessChannelMode(QProcess::MergedChannels);
    if (!workingDir.isEmpty()) m_debugger->setWorkingDirectory(workingDir);
    connect(m_debugger, &QProcess::readyReadStandardOutput, this, &BottomPanel::onDebugOutput);
    connect(m_debugger, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
                m_debugView->appendPlainText(tr("── Debug session ended (exit code %1) ──")
                                                 .arg(code));
            });
    connect(m_debugger, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            m_debugView->appendPlainText(tr("Could not start the debugger."));
    });

#ifdef Q_OS_WIN
    m_debugger->start(QStringLiteral("cmd.exe"), { QStringLiteral("/c"), command });
#else
    m_debugger->start(QStringLiteral("/bin/sh"), { QStringLiteral("-c"), command });
#endif
    m_debugInput->setFocus();
}

void BottomPanel::onDebugOutput()
{
    QTextCursor c(m_debugView->document());
    c.movePosition(QTextCursor::End);
    c.insertText(QString::fromLocal8Bit(m_debugger->readAllStandardOutput()));
    m_debugView->verticalScrollBar()->setValue(m_debugView->verticalScrollBar()->maximum());
}

void BottomPanel::onDebugInput()
{
    const QString expr = m_debugInput->text();
    m_debugInput->clear();
    if (expr.trimmed().isEmpty()) return;

    m_debugView->appendPlainText(QStringLiteral("❯ ") + expr);
    if (!m_debugger || m_debugger->state() != QProcess::Running) {
        m_debugView->appendPlainText(tr("No active debug session."));
        return;
    }
    m_debugger->write((expr + QLatin1Char('\n')).toUtf8());
}

// ─────────────────────────────────────────────────────────────────────────────
// TERMINAL
// ─────────────────────────────────────────────────────────────────────────────

QWidget *BottomPanel::createTerminalView()
{
    m_terminals = new QStackedWidget(this);
    // The header (and its picker) is built after the views, so the first
    // terminal is added directly and listed in createHeader().
    m_terminals->addWidget(new TerminalWidget(m_terminals));
    m_terminalSeq = 1;
    return m_terminals;
}

TerminalWidget *BottomPanel::currentTerminal() const
{
    return qobject_cast<TerminalWidget *>(m_terminals->currentWidget());
}

TerminalWidget *BottomPanel::addTerminal()
{
    auto *term = new TerminalWidget(m_terminals);
    m_terminals->addWidget(term);
    m_terminalPick->addItem(QStringLiteral("%1: bash").arg(++m_terminalSeq));
    m_terminalPick->setCurrentIndex(m_terminalPick->count() - 1);
    showView(Terminal);
    return term;
}

void BottomPanel::killTerminal()
{
    const int idx = m_terminals->currentIndex();
    if (idx < 0) return;
    QWidget *w = m_terminals->widget(idx);
    m_terminals->removeWidget(w);
    m_terminalPick->removeItem(idx);
    w->deleteLater();   // TerminalWidget's destructor stops bash

    if (m_terminals->count() == 0)
        addTerminal();
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTS
// ─────────────────────────────────────────────────────────────────────────────

QWidget *BottomPanel::createPortsView()
{
    auto *w  = new QWidget(this);
    auto *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    m_ports = new QTableWidget(0, 4, w);
    m_ports->setHorizontalHeaderLabels({ tr("Port"), tr("Forwarded Address"),
                                         tr("Running Process"), tr("Origin") });
    m_ports->verticalHeader()->hide();
    m_ports->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ports->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ports->setFrameShape(QFrame::NoFrame);
    m_ports->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_ports->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    m_portsEmpty = new QLabel(tr("No forwarded ports. Forward a port to access your "
                                 "running services locally."), w);
    m_portsEmpty->setAlignment(Qt::AlignCenter);
    m_portsEmpty->setWordWrap(true);

    auto *addBtn = new QPushButton(tr("Forward a Port"), w);
    connect(addBtn, &QPushButton::clicked, this, &BottomPanel::addForwardedPort);
    auto *btnRow = new QHBoxLayout;
    btnRow->setContentsMargins(8, 6, 8, 6);
    btnRow->addWidget(addBtn);
    btnRow->addStretch();

    vl->addWidget(m_ports, 1);
    vl->addWidget(m_portsEmpty, 1);
    vl->addLayout(btnRow);
    m_ports->hide();
    return w;
}

// Lists local TCP listeners (via lsof) and keeps any user-forwarded rows.
void BottomPanel::refreshPorts()
{
    for (int r = m_ports->rowCount() - 1; r >= 0; --r)
        if (!m_ports->item(r, 0)->data(Qt::UserRole).toBool())
            m_ports->removeRow(r);

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int, QProcess::ExitStatus) {
        const QStringList lines = QString::fromLocal8Bit(proc->readAllStandardOutput())
                                      .split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        static const QRegularExpression ws(QStringLiteral("\\s+"));
        QSet<QString> seen;
        for (int i = 1; i < lines.size(); ++i) {   // skip header row
            const QStringList f = lines.at(i).split(ws, Qt::SkipEmptyParts);
            if (f.size() < 9) continue;
            const QString name = f.at(8);               // e.g. *:8080 or 127.0.0.1:5173
            const int colon    = name.lastIndexOf(QLatin1Char(':'));
            if (colon < 0) continue;
            const QString port = name.mid(colon + 1);
            if (seen.contains(port)) continue;          // IPv4 + IPv6 duplicates
            seen.insert(port);

            QString host = name.left(colon);
            if (host == QLatin1String("*") || host == QLatin1String("[::]"))
                host = QStringLiteral("localhost");

            const int r = m_ports->rowCount();
            m_ports->insertRow(r);
            m_ports->setItem(r, 0, new QTableWidgetItem(port));
            m_ports->setItem(r, 1, new QTableWidgetItem(host + QLatin1Char(':') + port));
            m_ports->setItem(r, 2, new QTableWidgetItem(
                QStringLiteral("%1 (pid %2)").arg(f.at(0), f.at(1))));
            m_ports->setItem(r, 3, new QTableWidgetItem(tr("Auto Forwarded")));
        }
        m_ports->sortItems(0);
        m_ports->setVisible(m_ports->rowCount() > 0);
        m_portsEmpty->setVisible(m_ports->rowCount() == 0);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, proc, [this, proc](QProcess::ProcessError) {
        m_ports->setVisible(m_ports->rowCount() > 0);
        m_portsEmpty->setVisible(m_ports->rowCount() == 0);
        proc->deleteLater();
    });
    proc->start(QStringLiteral("lsof"),
                { QStringLiteral("-nP"), QStringLiteral("-iTCP"),
                  QStringLiteral("-sTCP:LISTEN") });
}

void BottomPanel::addForwardedPort()
{
    bool ok = false;
    const int port = QInputDialog::getInt(this, tr("Forward a Port"),
                                          tr("Port number:"), 8080, 1, 65535, 1, &ok);
    if (!ok) return;

    const int r = m_ports->rowCount();
    m_ports->insertRow(r);
    auto *portItem = new QTableWidgetItem(QString::number(port));
    portItem->setData(Qt::UserRole, true);   // user-added: survives refresh
    m_ports->setItem(r, 0, portItem);
    m_ports->setItem(r, 1, new QTableWidgetItem(QStringLiteral("localhost:%1").arg(port)));
    m_ports->setItem(r, 2, new QTableWidgetItem(QString()));
    m_ports->setItem(r, 3, new QTableWidgetItem(tr("User Forwarded")));
    m_ports->show();
    m_portsEmpty->hide();
}
