#pragma once
#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QListWidget>
#include "TestProcess.h"
#include "SyntaxHighlighter.h"

class MultiTestProcess;

class TestRunnerWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit TestRunnerWindow(QWidget *parent = nullptr);
    ~TestRunnerWindow() override = default;

    void setBinaryPath(const QString &path);

private slots:
    void onAddFile();
    void onRemoveFile();
    void onRun();
    void onStop();
    void onResult(const TestResult &result);
    void onLogLine(const QString &html);
    void onStarted(const QString &language);
    void onFinished(int exitCode);
    void onFileListChanged(int row);
    void onTreeItemClicked(QTreeWidgetItem *item, int col);
    void onFilterChanged(const QString &text);
    void onTabChanged(int index);

private:
    void buildUi();
    void applyStyle();
    void resetResults();
    void updateSummary();
    void loadSourceFile(const QString &path);
    QTreeWidgetItem *getOrCreateSuiteItem(const QString &name);
    QString languageFromPath(const QString &path);
    SyntaxHighlighter::Language highlighterLang(const QString &path);

    // ── Widgets ───────────────────────────────────────────────────────────────
    QListWidget    *m_fileList    = nullptr;
    QPushButton    *m_addBtn      = nullptr;
    QPushButton    *m_removeBtn   = nullptr;
    QPushButton    *m_runBtn      = nullptr;
    QPushButton    *m_stopBtn     = nullptr;
    QLineEdit      *m_filterEdit  = nullptr;
    QProgressBar   *m_progress    = nullptr;
    QLabel         *m_summaryLbl  = nullptr;
    QTabWidget     *m_tabs        = nullptr;
    QTreeWidget    *m_tree        = nullptr;
    QPlainTextEdit *m_srcView     = nullptr;
    QTextEdit      *m_logView     = nullptr;

    // ── State ─────────────────────────────────────────────────────────────────
    MultiTestProcess  *m_multiProc   = nullptr;
    TestProcess       *m_qtProc      = nullptr;
    QMap<QString, QTreeWidgetItem *> m_suiteItems;
    int m_total = 0, m_passed = 0, m_failed = 0, m_skipped = 0;
    int m_filesDone = 0, m_filesTotal = 0;
    QStringList m_pendingFiles;
    SyntaxHighlighter *m_highlighter = nullptr;
};
