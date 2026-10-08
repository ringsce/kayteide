#ifndef BOTTOMPANEL_H
#define BOTTOMPANEL_H

#include <QWidget>
#include <QProcess>
#include <QVector>

class QButtonGroup;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;
class QTableWidget;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class TerminalWidget;

// ─── BottomPanel ──────────────────────────────────────────────────────────────
// VSCodium-style bottom panel with five views selected from a flat tab strip:
//   PROBLEMS · OUTPUT · DEBUG CONSOLE · TERMINAL · PORTS
// The header also carries per-view actions (new / kill terminal, clear,
// refresh) plus maximise and close buttons, like the VS Code panel.
class BottomPanel : public QWidget
{
    Q_OBJECT
public:
    enum View { Problems = 0, Output, DebugConsole, Terminal, Ports };

    explicit BottomPanel(QWidget *parent = nullptr);
    ~BottomPanel() override;

    void showView(View view);
    View currentView() const;

    // ── Terminal ──────────────────────────────────────────────────────────────
    TerminalWidget *currentTerminal() const;
    TerminalWidget *addTerminal();

    // ── Output: run a shell command, stream it to the given output channel and
    //    collect compiler diagnostics into PROBLEMS. ──────────────────────────
    void runTask(const QString &channel, const QString &command,
                 const QString &workingDir);
    void appendOutput(const QString &channel, const QString &text);

    // ── Problems ──────────────────────────────────────────────────────────────
    void clearProblems();
    void addProblem(const QString &file, int line, int column,
                    const QString &severity, const QString &message);

    // ── Debug console: start a debugger (e.g. lldb) and pipe its I/O ──────────
    void startDebugger(const QString &command, const QString &workingDir);

signals:
    void problemActivated(const QString &file, int line, int column);
    // Every diagnostic as it is collected, and when the list is reset
    // (a new task starts) — the editor marks the lines.
    void problemAdded(const QString &file, int line, int column,
                      const QString &severity, const QString &message);
    void problemsCleared();
    // A runTask() task ended (not emitted for one replaced by a new task).
    void taskFinished(int exitCode, bool crashed);
    void closeRequested();
    void maximizeToggled(bool maximized);

private slots:
    void onTaskOutput();
    void onTaskFinished(int exitCode, QProcess::ExitStatus status);
    void onDebugOutput();
    void onDebugInput();
    void refreshPorts();
    void addForwardedPort();
    void onProblemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    QWidget *createHeader();
    QWidget *createProblemsView();
    QWidget *createOutputView();
    QWidget *createDebugConsoleView();
    QWidget *createTerminalView();
    QWidget *createPortsView();
    void     updateHeaderActions();
    void     updateProblemsBadge();
    void     parseDiagnostics(const QString &text);
    void     killTerminal();
    QPlainTextEdit *makeConsole();

    // header
    QButtonGroup   *m_tabGroup      { nullptr };
    QToolButton    *m_problemsTab   { nullptr };
    QComboBox      *m_outputChannel { nullptr };
    QComboBox      *m_terminalPick  { nullptr };
    QToolButton    *m_btnNewTerm    { nullptr };
    QToolButton    *m_btnKillTerm   { nullptr };
    QToolButton    *m_btnClear      { nullptr };
    QToolButton    *m_btnRefresh    { nullptr };
    QToolButton    *m_btnMaximize   { nullptr };
    QStackedWidget *m_stack         { nullptr };

    // problems
    QTreeWidget    *m_problems      { nullptr };
    int             m_errorCount    { 0 };
    int             m_warningCount  { 0 };

    // output
    QStackedWidget *m_outputStack   { nullptr };
    QProcess       *m_task          { nullptr };
    QString         m_taskChannel;
    QString         m_taskWorkingDir;
    QString         m_taskPending;   // partial line carried between reads

    // debug console
    QPlainTextEdit *m_debugView     { nullptr };
    QLineEdit      *m_debugInput    { nullptr };
    QProcess       *m_debugger      { nullptr };

    // terminal
    QStackedWidget *m_terminals     { nullptr };
    int             m_terminalSeq   { 0 };

    // ports
    QTableWidget   *m_ports         { nullptr };
    QLabel         *m_portsEmpty    { nullptr };
};

#endif // BOTTOMPANEL_H
