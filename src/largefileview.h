#ifndef LARGEFILEVIEW_H
#define LARGEFILEVIEW_H

#include <QAbstractScrollArea>
#include <QByteArray>
#include <QFile>
#include <QMutex>
#include <QVector>
#include <QWidget>

#include <atomic>

class QLabel;
class QLineEdit;
class QThread;

// ─── LargeFileView ────────────────────────────────────────────────────────────
// Editor for files far too big for QPlainTextEdit (10M+ lines, multi-GB).
//
// RAM: the file is memory-mapped and never copied. A worker thread indexes
// line starts sparsely (one offset per 64 lines, ~1.25 MB for 10M lines) and
// releases the pages it has scanned. Edits live in a line-based piece table:
// the document is a list of pieces that are either a run of original lines
// (two integers) or a few edited lines held in memory. Editing one line costs
// one line of RAM; deleting a million lines costs a few bytes. Undo/redo keep
// pieces too, never copies of file text.
//
// CPU: only on-screen rows are decoded and painted; the caret doesn't blink;
// indexing, search and save run on low-priority threads that exit when done,
// so an idle view uses 0% CPU.
//
// Save: streams the pieces (original runs straight from the mapping) to a
// temporary file on a worker thread, swaps it in atomically, then remaps.
class LargeFileView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit LargeFileView(QWidget *parent = nullptr);
    ~LargeFileView() override;

    bool    open(const QString &path, QString *error = nullptr);
    QString filePath() const { return m_path; }

    qint64  lineCount() const;
    bool    isIndexing() const { return !m_indexDone.load(); }
    bool    isSaving() const   { return m_saving.load(); }
    bool    isEditable() const { return m_indexDone.load() && !m_saving.load(); }
    bool    isModified() const { return m_modified; }
    qint64  fileSize() const   { return m_size; }

    void    goToLine(qint64 line);                 // 1-based
    void    setSelection(qint64 anchorLine, qint64 cursorLine);   // whole lines, 1-based
    qint64  currentLine() const   { return m_cursorLine + 1; }
    int     currentColumn() const { return m_cursorCol + 1; }

    void    findNext(const QString &text, Qt::CaseSensitivity cs);
    void    cancelFind();

    void    save();                                // asynchronous
    void    undo();
    void    redo();

signals:
    void indexProgress(qint64 linesSoFar, int percent);
    void indexFinished(qint64 lines, qint64 msecs);
    void findResult(qint64 line, bool found);     // line 1-based
    void cursorMoved(qint64 line);
    void modificationChanged(bool modified);
    void saveRequested();                          // Ctrl+S pressed in the view
    void saveProgress(int percent);
    void saveFinished(bool ok, const QString &error);

protected:
    bool event(QEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void inputMethodEvent(QInputMethodEvent *) override;
    void changeEvent(QEvent *) override;

private:
    // A run of original lines (orig = true: [start, start+count)), or lines
    // held in memory (orig = false: `lines`, count == lines.size()).
    struct Piece {
        bool               orig  { true };
        qint64             start { 0 };
        qint64             count { 0 };
        QVector<QByteArray> lines;
    };
    // An undoable change: replace `removeCount` lines at `line` with `insert`.
    struct Edit {
        qint64          line { 0 };
        qint64          removeCount { 0 };
        QVector<Piece>  insert;
        qint64          cursorLine { 0 };
        int             cursorCol  { 0 };
        bool            typing { false };          // coalesces with next keystrokes
        qint64          stamp  { 0 };
    };

    // ── Original file (memory-mapped) ──
    bool       mapFile(QString *error);
    void       startIndexing();
    qint64     origLineStart(qint64 line) const;
    qint64     origContentEnd(qint64 line) const;  // end of text, before \r\n / \n
    QByteArray origLine(qint64 line, qint64 cap) const;
    qint64     origLineForOffset(qint64 offset) const;

    // ── Piece table ──
    void       resetPieces();
    void       rebuildPrefix();
    QPair<int, qint64> locate(qint64 line) const;
    int        splitAt(qint64 line);
    QVector<Piece> replace(qint64 line, qint64 removeCount, const QVector<Piece> &insert);
    QByteArray docLine(qint64 line, qint64 cap = -1) const;
    QString    lineText(qint64 line) const { return QString::fromUtf8(docLine(line)); }
    static Piece memPiece(const QVector<QByteArray> &lines);
    static qint64 piecesLines(const QVector<Piece> &pieces);

    // ── Editing ──
    void       applyEdit(const Edit &e, QVector<Edit> &inverseStack);
    void       commitReplace(qint64 line, qint64 removeCount, const QVector<QByteArray> &newLines,
                             qint64 cursorLine, int cursorCol);
    void       setLineText(qint64 line, const QString &text, int newCol);
    void       insertText(const QString &text);
    void       deleteSelectedLines();
    bool       hasLineSelection() const { return m_anchorLine != m_cursorLine; }
    void       setModified(bool m);
    void       finishSave(bool ok, const QString &error);

    // ── View ──
    void       updateScrollBars();
    void       ensureCursorVisible();
    void       copySelection();
    int        gutterWidth() const;
    int        visibleRows() const;
    int        visualColumn(const QString &text, int col) const;
    int        columnForVisual(const QString &text, int visual) const;

    static constexpr int kStride = 64;            // lines per index checkpoint

    QString          m_path;
    QFile            m_file;
    const uchar     *m_data { nullptr };
    qint64           m_size { 0 };
    QByteArray       m_eol { "\n" };

    // Shared with worker threads.
    mutable QMutex       m_indexMutex;
    QVector<qint64>      m_checkpoints;           // offset of original line i*kStride
    std::atomic<qint64>  m_lines { 1 };           // original lines (grows while indexing)
    std::atomic<bool>    m_indexDone { false };
    std::atomic<bool>    m_stop { false };
    std::atomic<bool>    m_saving { false };
    std::atomic<bool>    m_findCancel { false };
    QThread             *m_indexer { nullptr };
    QThread             *m_finder  { nullptr };
    QThread             *m_saver   { nullptr };

    // Document (GUI thread only; workers get copies).
    QVector<Piece>   m_pieces;
    QVector<qint64>  m_prefix;                    // m_prefix[i] = lines before piece i
    qint64           m_docLines { 1 };
    QVector<Edit>    m_undo, m_redo;
    int              m_cleanUndo { 0 };           // undo depth matching the saved file
    bool             m_modified { false };

    // View state
    qint64  m_cursorLine { 0 };
    int     m_cursorCol  { 0 };
    qint64  m_anchorLine { 0 };
    int     m_lineHeight { 16 };
    int     m_charWidth  { 8 };
    int     m_maxColumns { 0 };
};

// ─── LargeFileTab ─────────────────────────────────────────────────────────────
// Editor-area tab around LargeFileView: info bar (size, lines, state), find bar
// (Ctrl+F), Go to line (Ctrl+G), Save (Ctrl+S).
class LargeFileTab : public QWidget
{
    Q_OBJECT
public:
    explicit LargeFileTab(QWidget *parent = nullptr);

    bool           open(const QString &path, QString *error = nullptr);
    QString        filePath() const   { return m_view->filePath(); }
    LargeFileView *view() const       { return m_view; }
    bool           isModified() const { return m_view->isModified(); }

    void save();                              // asynchronous, progress in the info bar
    bool saveAndWait();                       // modal progress; for close / quit

    // Files at least this big open here instead of the normal editor.
    static constexpr qint64 kThresholdBytes = 32LL * 1024 * 1024;

signals:
    void modificationChanged(bool modified);

private:
    void updateInfo();
    void showFind();
    void promptGoToLine();

    LargeFileView *m_view   { nullptr };
    QLabel        *m_info   { nullptr };
    QWidget       *m_findBar { nullptr };
    QLineEdit     *m_findEdit { nullptr };
    QLabel        *m_findStatus { nullptr };
    qint64         m_indexMs { -1 };
    int            m_savePct { -1 };
    QString        m_lastError;
};

#endif // LARGEFILEVIEW_H
