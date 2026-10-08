#include "largefileview.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDateTime>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPainter>
#include <QProgressDialog>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QShortcut>
#include <QThread>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>

#ifdef Q_OS_UNIX
#  include <sys/mman.h>
#  include <unistd.h>
#endif

namespace {

constexpr qint64 kReleaseChunk    = 64LL * 1024 * 1024;  // drop scanned pages every 64 MB
constexpr qint64 kMaxDisplayBytes = 64 * 1024;           // per line; minified files can be huge
constexpr qint64 kMaxCopyBytes    = 64LL * 1024 * 1024;  // clipboard cap
constexpr qint64 kFindBlock       = 16LL * 1024 * 1024;  // search granularity (cancel checks)
constexpr qint64 kWriteChunk      = 16LL * 1024 * 1024;  // save granularity
constexpr qint64 kTypingMergeMs   = 1500;                // keystrokes merged into one undo step

// Tell the kernel it may drop these file-backed pages; they are re-read from
// disk if needed again. Keeps resident memory flat while scanning a huge file.
void releasePages(const uchar *base, qint64 from, qint64 to)
{
#ifdef Q_OS_UNIX
    if (!base) return;
    static const qint64 page = ::sysconf(_SC_PAGESIZE);
    from = (from / page) * page;
    to   = (to / page) * page;
    if (to > from) {
        void *addr = const_cast<uchar *>(base) + from;
        ::madvise(addr, size_t(to - from), MADV_DONTNEED);
#  ifdef Q_OS_MACOS
        // On macOS MADV_DONTNEED is only a hint for file-backed pages;
        // invalidating the (read-only, clean) mapping actually drops them.
        ::msync(addr, size_t(to - from), MS_INVALIDATE);
#  endif
    }
#else
    Q_UNUSED(base) Q_UNUSED(from) Q_UNUSED(to)
#endif
}

void advise(const uchar *base, qint64 size, bool sequential)
{
#ifdef Q_OS_UNIX
    if (base && size > 0)
        ::madvise(const_cast<uchar *>(base), size_t(size), sequential ? MADV_SEQUENTIAL : MADV_NORMAL);
#else
    Q_UNUSED(base) Q_UNUSED(size) Q_UNUSED(sequential)
#endif
}

QString formatBytes(qint64 b)
{
    return QLocale().formattedDataSize(b, 1, QLocale::DataSizeTraditionalFormat);
}

// Search [from, to) of a byte buffer, in blocks so cancellation is quick.
qint64 searchBytes(const char *base, qint64 from, qint64 to, const QByteArray &needle,
                   bool icase, const std::atomic<bool> &cancel, const uchar *releaseBase)
{
    const auto eqI = [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == b; };
    for (qint64 b = from; b < to && !cancel; b += kFindBlock) {
        const qint64 e = qMin(to, b + kFindBlock + needle.size() - 1);
        const char *hit;
        if (icase) {
            hit = std::search(base + b, base + e, needle.cbegin(), needle.cend(), eqI);
        } else {
#ifdef Q_OS_UNIX
            // libc's memmem is vectorised: several GB/s.
            const void *m = ::memmem(base + b, size_t(e - b), needle.constData(), size_t(needle.size()));
            hit = m ? static_cast<const char *>(m) : base + e;
#else
            hit = std::search(base + b, base + e,
                              std::boyer_moore_horspool_searcher(needle.cbegin(), needle.cend()));
#endif
        }
        if (hit != base + e) return hit - base;
        releasePages(releaseBase, b, e);
    }
    return -1;
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// LargeFileView — construction, mapping, indexing
// ═════════════════════════════════════════════════════════════════════════════

LargeFileView::LargeFileView(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(qMax(11, mono.pointSize()));
    setFont(mono);
    viewport()->setFont(mono);
    const QFontMetrics fm(mono);
    m_lineHeight = fm.lineSpacing();
    m_charWidth  = qMax(1, fm.horizontalAdvance(QLatin1Char('M')));

    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    viewport()->setCursor(Qt::IBeamCursor);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, viewport(), qOverload<>(&QWidget::update));
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, viewport(), qOverload<>(&QWidget::update));
    resetPieces();
}

LargeFileView::~LargeFileView()
{
    m_stop = true;
    m_findCancel = true;
    for (QThread *t : { m_indexer, m_finder, m_saver }) {
        if (t) { t->wait(); delete t; }
    }
    if (m_data) m_file.unmap(const_cast<uchar *>(m_data));
}

bool LargeFileView::open(const QString &path, QString *error)
{
    m_path = path;
    if (!mapFile(error)) return false;
    startIndexing();
    updateScrollBars();
    return true;
}

bool LargeFileView::mapFile(QString *error)
{
    if (m_data) { m_file.unmap(const_cast<uchar *>(m_data)); m_data = nullptr; }
    m_file.close();
    m_file.setFileName(m_path);
    if (!m_file.open(QIODevice::ReadOnly)) {
        if (error) *error = m_file.errorString();
        return false;
    }
    m_size = m_file.size();
    if (m_size > 0) {
        m_data = m_file.map(0, m_size);
        if (!m_data) {
            if (error) *error = tr("Could not memory-map the file: %1").arg(m_file.errorString());
            return false;
        }
        // Keep the file's line-ending style for lines we write.
        const qint64 probe = qMin<qint64>(m_size, 1 << 20);
        const void *nl = std::memchr(m_data, '\n', size_t(probe));
        m_eol = (nl && nl > m_data && static_cast<const uchar *>(nl)[-1] == '\r')
                    ? QByteArray("\r\n") : QByteArray("\n");
    }
    return true;
}

void LargeFileView::startIndexing()
{
    if (m_indexer) { m_indexer->wait(); delete m_indexer; m_indexer = nullptr; }
    {
        QMutexLocker lock(&m_indexMutex);
        m_checkpoints = { 0 };
    }
    m_lines = 1;
    m_stop = false;
    m_indexDone = false;
    resetPieces();

    if (m_size == 0) {
        m_indexDone = true;
        QMetaObject::invokeMethod(this, [this] { emit indexFinished(1, 0); }, Qt::QueuedConnection);
        return;
    }

    // Line starts are found on a low-priority worker thread.
    m_indexer = QThread::create([this] {
        QElapsedTimer total, progress;
        total.start(); progress.start();
        advise(m_data, m_size, true);

        const uchar *p = m_data;
        const qint64 n = m_size;
        qint64 pos = 0, newlines = 0, released = 0;
        QVector<qint64> batch;
        batch.reserve(16384);

        auto publish = [&] {
            QMutexLocker lock(&m_indexMutex);
            m_checkpoints += batch;
            batch.clear();
            m_lines.store(newlines + 1);
        };

        while (pos < n && !m_stop.load(std::memory_order_relaxed)) {
            const void *hit = std::memchr(p + pos, '\n', size_t(n - pos));
            if (!hit) break;
            pos = static_cast<const uchar *>(hit) - p + 1;
            ++newlines;
            if (newlines % kStride == 0) batch.append(pos);

            if (pos - released >= kReleaseChunk) {
                releasePages(p, released, pos);
                released = pos;
                publish();
                if (progress.elapsed() >= 120) {        // ~8 updates/s, no more
                    progress.restart();
                    const qint64 lines = newlines + 1;
                    const int pct = int(pos * 100 / n);
                    QMetaObject::invokeMethod(this, [this, lines, pct] {
                        updateScrollBars();
                        viewport()->update();
                        emit indexProgress(lines, pct);
                    }, Qt::QueuedConnection);
                }
            }
        }
        publish();
        releasePages(p, released, n);
        advise(m_data, m_size, false);
        const qint64 lines = newlines + 1, ms = total.elapsed();
        // Results always reach the GUI thread, whoever listens.
        QMetaObject::invokeMethod(this, [this, lines, ms] {
            m_indexDone = true;
            resetPieces();                               // the document = all original lines
            m_cursorLine = qMin(m_cursorLine, m_docLines - 1);
            m_anchorLine = m_cursorLine;
            updateScrollBars();
            viewport()->update();
            emit indexFinished(lines, ms);
        }, Qt::QueuedConnection);
    });
    m_indexer->start(QThread::LowPriority);
}

qint64 LargeFileView::origLineStart(qint64 line) const
{
    if (line <= 0 || !m_data) return 0;
    qint64 offset;
    {
        QMutexLocker lock(&m_indexMutex);
        const qint64 cp = qMin<qint64>(line / kStride, m_checkpoints.size() - 1);
        offset = m_checkpoints.at(cp);
        line -= cp * kStride;
    }
    // At most kStride-1 newlines to skip from the checkpoint.
    while (line-- > 0 && offset < m_size) {
        const void *hit = std::memchr(m_data + offset, '\n', size_t(m_size - offset));
        if (!hit) return m_size;
        offset = static_cast<const uchar *>(hit) - m_data + 1;
    }
    return offset;
}

qint64 LargeFileView::origContentEnd(qint64 line) const
{
    const qint64 start = origLineStart(line);
    if (start >= m_size) return m_size;
    const void *hit = std::memchr(m_data + start, '\n', size_t(m_size - start));
    qint64 end = hit ? static_cast<const uchar *>(hit) - m_data : m_size;
    if (end > start && m_data[end - 1] == '\r') --end;
    return end;
}

QByteArray LargeFileView::origLine(qint64 line, qint64 cap) const
{
    const qint64 start = origLineStart(line);
    if (start >= m_size) return {};
    const qint64 limit = cap < 0 ? m_size - start : qMin(m_size - start, cap + 1);
    const void *hit = std::memchr(m_data + start, '\n', size_t(limit));
    qint64 end = hit ? static_cast<const uchar *>(hit) - m_data : start + limit;
    const bool clipped = !hit && start + limit < m_size;
    if (end > start && m_data[end - 1] == '\r') --end;
    QByteArray out(reinterpret_cast<const char *>(m_data + start),
                   cap < 0 ? end - start : qMin(end - start, cap));
    if (clipped) out += " …";
    return out;
}

qint64 LargeFileView::origLineForOffset(qint64 offset) const
{
    qint64 line, from;
    {
        QMutexLocker lock(&m_indexMutex);
        auto it = std::upper_bound(m_checkpoints.cbegin(), m_checkpoints.cend(), offset);
        const qint64 cp = qMax<qint64>(0, (it - m_checkpoints.cbegin()) - 1);
        line = cp * kStride;
        from = m_checkpoints.at(cp);
    }
    while (from < offset) {
        const void *hit = std::memchr(m_data + from, '\n', size_t(offset - from));
        if (!hit) break;
        from = static_cast<const uchar *>(hit) - m_data + 1;
        ++line;
    }
    return line;
}

// ═════════════════════════════════════════════════════════════════════════════
// Piece table
// ═════════════════════════════════════════════════════════════════════════════

LargeFileView::Piece LargeFileView::memPiece(const QVector<QByteArray> &lines)
{
    Piece p;
    p.orig  = false;
    p.count = lines.size();
    p.lines = lines;
    return p;
}

qint64 LargeFileView::piecesLines(const QVector<Piece> &pieces)
{
    qint64 n = 0;
    for (const Piece &p : pieces) n += p.count;
    return n;
}

void LargeFileView::resetPieces()
{
    m_pieces = { Piece { true, 0, m_lines.load(), {} } };
    m_undo.clear();
    m_redo.clear();
    m_cleanUndo = 0;
    rebuildPrefix();
    setModified(false);
}

void LargeFileView::rebuildPrefix()
{
    // Merge neighbours so the table stays short: contiguous original runs and
    // adjacent in-memory pieces.
    QVector<Piece> merged;
    merged.reserve(m_pieces.size());
    for (const Piece &p : std::as_const(m_pieces)) {
        if (p.count <= 0) continue;
        if (!merged.isEmpty()) {
            Piece &last = merged.last();
            if (last.orig && p.orig && last.start + last.count == p.start) {
                last.count += p.count;
                continue;
            }
            if (!last.orig && !p.orig) {
                last.lines += p.lines;
                last.count += p.count;
                continue;
            }
        }
        merged.append(p);
    }
    if (merged.isEmpty()) merged.append(memPiece({ QByteArray() }));   // never empty: one blank line
    m_pieces = std::move(merged);

    m_prefix.resize(m_pieces.size() + 1);
    qint64 total = 0;
    for (int i = 0; i < m_pieces.size(); ++i) {
        m_prefix[i] = total;
        total += m_pieces.at(i).count;
    }
    m_prefix[m_pieces.size()] = total;
    m_docLines = total;
}

QPair<int, qint64> LargeFileView::locate(qint64 line) const
{
    auto it = std::upper_bound(m_prefix.cbegin(), m_prefix.cend() - 1, line);
    const int idx = qMax(0, int(it - m_prefix.cbegin()) - 1);
    return { idx, line - m_prefix.at(idx) };
}

int LargeFileView::splitAt(qint64 line)
{
    if (line >= m_docLines) return m_pieces.size();
    const auto [idx, off] = locate(line);
    if (off == 0) return idx;
    Piece &p = m_pieces[idx];
    Piece tail;
    if (p.orig) {
        tail = Piece { true, p.start + off, p.count - off, {} };
    } else {
        tail = memPiece(p.lines.mid(off));
        p.lines.resize(off);
    }
    p.count = off;
    m_pieces.insert(idx + 1, tail);
    // Only the new piece needs a prefix entry; the others are unchanged.
    m_prefix.insert(idx + 1, line);
    return idx + 1;
}

QVector<LargeFileView::Piece> LargeFileView::replace(qint64 line, qint64 removeCount,
                                                       const QVector<Piece> &insert)
{
    const int a = splitAt(line);
    const int b = splitAt(line + removeCount);
    QVector<Piece> removed = m_pieces.mid(a, b - a);
    m_pieces.remove(a, b - a);
    for (int i = 0; i < insert.size(); ++i) m_pieces.insert(a + i, insert.at(i));
    rebuildPrefix();
    return removed;
}

qint64 LargeFileView::lineCount() const
{
    return m_indexDone.load() ? m_docLines : m_lines.load();
}

QByteArray LargeFileView::docLine(qint64 line, qint64 cap) const
{
    if (!m_indexDone.load()) return origLine(line, cap);     // before the table exists
    if (line < 0 || line >= m_docLines) return {};
    const auto [idx, off] = locate(line);
    const Piece &p = m_pieces.at(idx);
    if (p.orig) return origLine(p.start + off, cap);
    const QByteArray &b = p.lines.at(off);
    return (cap >= 0 && b.size() > cap) ? b.left(cap) + " …" : b;
}

// ═════════════════════════════════════════════════════════════════════════════
// Editing
// ═════════════════════════════════════════════════════════════════════════════

void LargeFileView::setModified(bool m)
{
    if (m_modified == m) return;
    m_modified = m;
    emit modificationChanged(m);
}

// Replace lines and record the inverse for undo.
void LargeFileView::commitReplace(qint64 line, qint64 removeCount, const QVector<QByteArray> &newLines,
                                  qint64 cursorLine, int cursorCol)
{
    const qint64 before = m_docLines;
    Edit undo;
    undo.line       = line;
    undo.cursorLine = m_cursorLine;
    undo.cursorCol  = m_cursorCol;
    undo.insert     = replace(line, removeCount, newLines.isEmpty() ? QVector<Piece>{}
                                                                    : QVector<Piece>{ memPiece(newLines) });
    // Lines now occupying the replaced range (replace() keeps one blank line
    // if everything was deleted).
    undo.removeCount = m_docLines - (before - removeCount);
    m_undo.append(undo);
    m_redo.clear();
    if (m_cleanUndo >= m_undo.size()) m_cleanUndo = -1;         // saved state unreachable now
    m_cursorLine = qBound<qint64>(0, cursorLine, m_docLines - 1);
    m_anchorLine = m_cursorLine;
    m_cursorCol  = cursorCol;
    setModified(true);
    updateScrollBars();
    ensureCursorVisible();
    viewport()->update();
    emit cursorMoved(m_cursorLine + 1);
}

// Single-line change while typing: keystrokes on the same line within a short
// time become one undo step, and an already in-memory line is edited in place.
void LargeFileView::setLineText(qint64 line, const QString &text, int newCol)
{
    const QByteArray bytes = text.toUtf8();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const bool merge = !m_undo.isEmpty() && m_undo.last().typing && m_undo.last().line == line &&
                       m_undo.last().removeCount == 1 && now - m_undo.last().stamp < kTypingMergeMs &&
                       m_cleanUndo != m_undo.size();
    const auto [idx, off] = locate(line);

    if (merge && !m_pieces.at(idx).orig) {
        m_pieces[idx].lines[off] = bytes;                     // in place, no table change
        m_undo.last().stamp = now;
        m_redo.clear();
    } else {
        commitReplace(line, 1, { bytes }, line, newCol);
        m_undo.last().typing = true;
        m_undo.last().stamp  = now;
    }
    m_cursorLine = m_anchorLine = line;
    m_cursorCol  = newCol;
    setModified(true);
    ensureCursorVisible();
    viewport()->update();
    emit cursorMoved(m_cursorLine + 1);
}

void LargeFileView::insertText(const QString &input)
{
    if (!isEditable() || input.isEmpty()) return;
    if (hasLineSelection()) deleteSelectedLines();

    QString text = input;
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n")).replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const QString cur = lineText(m_cursorLine);
    const int col = qMin<int>(m_cursorCol, cur.size());
    const QString left = cur.left(col), right = cur.mid(col);

    if (!text.contains(QLatin1Char('\n'))) {
        setLineText(m_cursorLine, left + text + right, col + text.size());
        return;
    }
    const QStringList parts = text.split(QLatin1Char('\n'));
    QVector<QByteArray> lines;
    lines.reserve(parts.size());
    for (int i = 0; i < parts.size(); ++i) {
        QString s = parts.at(i);
        if (i == 0) s.prepend(left);
        if (i == parts.size() - 1) s.append(right);
        lines.append(s.toUtf8());
    }
    commitReplace(m_cursorLine, 1, lines, m_cursorLine + parts.size() - 1, int(parts.last().size()));
}

void LargeFileView::deleteSelectedLines()
{
    const qint64 from = qMin(m_anchorLine, m_cursorLine);
    const qint64 to   = qMax(m_anchorLine, m_cursorLine);
    commitReplace(from, to - from + 1, {}, from, 0);
}

void LargeFileView::applyEdit(const Edit &e, QVector<Edit> &inverseStack)
{
    Edit inv;
    inv.line        = e.line;
    inv.removeCount = piecesLines(e.insert);
    inv.cursorLine  = m_cursorLine;
    inv.cursorCol   = m_cursorCol;
    inv.insert      = replace(e.line, e.removeCount, e.insert);
    inverseStack.append(inv);
    m_cursorLine = m_anchorLine = qBound<qint64>(0, e.cursorLine, m_docLines - 1);
    m_cursorCol  = e.cursorCol;
    updateScrollBars();
    ensureCursorVisible();
    viewport()->update();
    emit cursorMoved(m_cursorLine + 1);
}

void LargeFileView::undo()
{
    if (!isEditable() || m_undo.isEmpty()) return;
    const Edit e = m_undo.takeLast();
    applyEdit(e, m_redo);
    setModified(m_undo.size() != m_cleanUndo);
}

void LargeFileView::redo()
{
    if (!isEditable() || m_redo.isEmpty()) return;
    const Edit e = m_redo.takeLast();
    applyEdit(e, m_undo);
    m_undo.last().typing = false;                 // don't merge new typing into a redone step
    setModified(m_undo.size() != m_cleanUndo);
}

// ═════════════════════════════════════════════════════════════════════════════
// Save
// ═════════════════════════════════════════════════════════════════════════════

void LargeFileView::save()
{
    if (!m_indexDone || m_saving) return;
    m_saving = true;
    viewport()->update();

    // The worker gets its own copy of the table (in-memory lines are shared,
    // not copied).
    const QVector<Piece> pieces = m_pieces;
    const qint64 totalLines = m_docLines;
    const QByteArray eol = m_eol;
    const QString path = m_path;

    if (m_saver) { m_saver->wait(); delete m_saver; m_saver = nullptr; }
    m_saver = QThread::create([this, pieces, totalLines, eol, path] {
        QSaveFile out(path);                 // temp file + atomic rename
        QString error;
        bool ok = out.open(QIODevice::WriteOnly);
        QElapsedTimer progress; progress.start();
        qint64 linesDone = 0;

        auto report = [&] {
            if (progress.elapsed() < 150) return;
            progress.restart();
            const int pct = int(linesDone * 100 / qMax<qint64>(1, totalLines));
            QMetaObject::invokeMethod(this, [this, pct] { emit saveProgress(pct); }, Qt::QueuedConnection);
        };

        for (int i = 0; ok && i < pieces.size() && !m_stop; ++i) {
            const Piece &p = pieces.at(i);
            if (i > 0) ok = out.write(eol) == eol.size();
            if (!ok) break;
            if (p.orig) {
                // Original run: copied straight from the mapping, in chunks.
                const qint64 from = origLineStart(p.start);
                const qint64 to   = origContentEnd(p.start + p.count - 1);
                const qint64 base = linesDone;
                for (qint64 b = from; ok && b < to; b += kWriteChunk) {
                    const qint64 len = qMin(kWriteChunk, to - b);
                    ok = out.write(reinterpret_cast<const char *>(m_data + b), len) == len;
                    releasePages(m_data, b, b + len);
                    linesDone = base + p.count * (b + len - from) / qMax<qint64>(1, to - from);
                    report();
                }
                linesDone = base + p.count;
            } else {
                for (int k = 0; ok && k < p.lines.size(); ++k) {
                    if (k > 0) ok = out.write(eol) == eol.size();
                    if (ok) ok = out.write(p.lines.at(k)) == p.lines.at(k).size();
                }
                linesDone += p.count;
                report();
            }
        }
        if (!ok || m_stop) {
            error = out.errorString();
            out.cancelWriting();
            ok = false;
        } else {
            ok = out.commit();
            if (!ok) error = out.errorString();
        }
        QMetaObject::invokeMethod(this, [this, ok, error] { finishSave(ok, error); },
                                  Qt::QueuedConnection);
    });
    m_saver->start(QThread::LowPriority);
}

void LargeFileView::finishSave(bool ok, const QString &error)
{
    m_saving = false;
    if (!ok) {
        viewport()->update();
        emit saveFinished(false, error);
        return;
    }
    cancelFind();                                     // it reads the mapping we're replacing
    // The file on disk is now the document: map it again and re-index.
    const qint64 line = m_cursorLine;
    const int    col  = m_cursorCol;
    const int    scroll = verticalScrollBar()->value();
    QString err;
    if (!mapFile(&err)) { emit saveFinished(false, err); return; }
    startIndexing();                                  // also resets pieces, undo, modified
    m_cursorLine = m_anchorLine = line;
    m_cursorCol  = col;
    updateScrollBars();
    verticalScrollBar()->setValue(scroll);
    emit saveFinished(true, QString());
}

// ═════════════════════════════════════════════════════════════════════════════
// View
// ═════════════════════════════════════════════════════════════════════════════

int LargeFileView::gutterWidth() const
{
    const int digits = QString::number(qMax<qint64>(1, lineCount())).size();
    return m_charWidth * (digits + 2);
}

int LargeFileView::visibleRows() const
{
    return qMax(1, viewport()->height() / m_lineHeight);
}

int LargeFileView::visualColumn(const QString &text, int col) const
{
    int v = 0;
    for (int i = 0; i < col && i < text.size(); ++i) v += text.at(i) == QLatin1Char('\t') ? 4 : 1;
    return v;
}

int LargeFileView::columnForVisual(const QString &text, int visual) const
{
    int v = 0;
    for (int i = 0; i < text.size(); ++i) {
        const int w = text.at(i) == QLatin1Char('\t') ? 4 : 1;
        if (v + w / 2.0 > visual) return i;
        v += w;
    }
    return text.size();
}

void LargeFileView::updateScrollBars()
{
    const qint64 lines = lineCount();
    const int rows = visibleRows();
    verticalScrollBar()->setPageStep(rows);
    verticalScrollBar()->setSingleStep(3);
    verticalScrollBar()->setRange(0, int(qMin<qint64>(INT_MAX, qMax<qint64>(0, lines - rows + 1))));

    const int cols = qMax(1, (viewport()->width() - gutterWidth()) / m_charWidth);
    horizontalScrollBar()->setPageStep(cols);
    horizontalScrollBar()->setRange(0, qMax(0, m_maxColumns - cols + 2));
}

void LargeFileView::ensureCursorVisible()
{
    QScrollBar *v = verticalScrollBar();
    const int rows = visibleRows();
    if (m_cursorLine < v->value())
        v->setValue(int(m_cursorLine));
    else if (m_cursorLine >= v->value() + rows - 1)
        v->setValue(int(qMax<qint64>(0, m_cursorLine - rows + 2)));

    const int vcol = visualColumn(lineText(m_cursorLine), m_cursorCol);
    QScrollBar *h = horizontalScrollBar();
    const int cols = qMax(1, (viewport()->width() - gutterWidth()) / m_charWidth);
    if (vcol + 2 > m_maxColumns) { m_maxColumns = vcol + 2; updateScrollBars(); }
    if (vcol < h->value()) h->setValue(vcol);
    else if (vcol >= h->value() + cols - 1) h->setValue(vcol - cols + 2);
}

void LargeFileView::paintEvent(QPaintEvent *)
{
    QPainter p(viewport());
    const QPalette pal = palette();
    p.fillRect(viewport()->rect(), pal.color(QPalette::Base));

    const int gutter = gutterWidth();
    const QColor gutterBg = pal.color(QPalette::Base).lightness() < 128
                                ? pal.color(QPalette::Base).lighter(118)
                                : pal.color(QPalette::Base).darker(106);
    p.fillRect(0, 0, gutter, viewport()->height(), gutterBg);

    QColor selection = pal.color(QPalette::Highlight);
    selection.setAlpha(70);
    QColor currentLine = pal.color(QPalette::Highlight);
    currentLine.setAlpha(22);
    const qint64 selFrom = qMin(m_anchorLine, m_cursorLine);
    const qint64 selTo   = qMax(m_anchorLine, m_cursorLine);

    const qint64 first = verticalScrollBar()->value();
    const qint64 lines = lineCount();
    const int    xOff  = horizontalScrollBar()->value() * m_charWidth;
    const int    ascent = QFontMetrics(font()).ascent();
    const int    textX = gutter + 4;
    int widest = m_maxColumns;

    for (int row = 0; row <= visibleRows(); ++row) {
        const qint64 line = first + row;
        if (line >= lines) break;
        const int y = row * m_lineHeight;

        if (hasLineSelection() && line >= selFrom && line <= selTo)
            p.fillRect(gutter, y, viewport()->width() - gutter, m_lineHeight, selection);
        else if (line == m_cursorLine)
            p.fillRect(gutter, y, viewport()->width() - gutter, m_lineHeight, currentLine);

        p.setPen(line == m_cursorLine ? pal.color(QPalette::Text)
                                      : pal.color(QPalette::PlaceholderText));
        p.drawText(QRect(0, y, gutter - m_charWidth, m_lineHeight),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(line + 1));

        const QString raw = QString::fromUtf8(docLine(line, kMaxDisplayBytes));
        QString text = raw;
        text.replace(QLatin1Char('\t'), QStringLiteral("    "));
        widest = qMax(widest, int(text.size()));
        p.setClipRect(gutter, y, viewport()->width() - gutter, m_lineHeight);
        p.setPen(pal.color(QPalette::Text));
        p.drawText(textX - xOff, y + ascent, text);

        // Solid caret (no blink timer: zero CPU while idle).
        if (line == m_cursorLine && hasFocus() && isEditable()) {
            const int cx = textX - xOff + visualColumn(raw, qMin<int>(m_cursorCol, raw.size())) * m_charWidth;
            p.fillRect(cx, y + 1, 2, m_lineHeight - 2, pal.color(QPalette::Text));
        }
        p.setClipping(false);
    }

    if (widest > m_maxColumns) {
        m_maxColumns = widest;
        QMetaObject::invokeMethod(this, [this] { updateScrollBars(); }, Qt::QueuedConnection);
    }
}

void LargeFileView::resizeEvent(QResizeEvent *e)
{
    QAbstractScrollArea::resizeEvent(e);
    updateScrollBars();
}

void LargeFileView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::PaletteChange || e->type() == QEvent::FontChange)
        viewport()->update();
    QAbstractScrollArea::changeEvent(e);
}

void LargeFileView::goToLine(qint64 line)
{
    const qint64 target = qBound<qint64>(0, line - 1, lineCount() - 1);
    m_cursorLine = m_anchorLine = target;
    m_cursorCol = 0;
    verticalScrollBar()->setValue(int(qMax<qint64>(0, target - visibleRows() / 2)));   // centre it
    horizontalScrollBar()->setValue(0);
    viewport()->update();
    emit cursorMoved(target + 1);
}

void LargeFileView::setSelection(qint64 anchorLine, qint64 cursorLine)
{
    const qint64 last = lineCount() - 1;
    m_anchorLine = qBound<qint64>(0, anchorLine - 1, last);
    m_cursorLine = qBound<qint64>(0, cursorLine - 1, last);
    m_cursorCol  = 0;
    ensureCursorVisible();
    viewport()->update();
    emit cursorMoved(m_cursorLine + 1);
}

// Claim editing keys before the main window's menu shortcuts (Copy, Paste,
// Undo, Save, …) so they act on this view while it has focus.
bool LargeFileView::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        auto *ke = static_cast<QKeyEvent *>(e);
        const bool plainText = !ke->text().isEmpty() && ke->text().at(0).isPrint() &&
                               !(ke->modifiers() & (Qt::ControlModifier | Qt::MetaModifier));
        if (plainText || ke->matches(QKeySequence::Copy) || ke->matches(QKeySequence::Cut) ||
            ke->matches(QKeySequence::Paste) || ke->matches(QKeySequence::Undo) ||
            ke->matches(QKeySequence::Redo) || ke->matches(QKeySequence::SelectAll) ||
            ke->matches(QKeySequence::Save)) {
            e->accept();
            return true;
        }
    }
    // Tab inserts a tab character instead of moving focus.
    if (e->type() == QEvent::KeyPress && isEditable()) {
        auto *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab && !(ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
            keyPressEvent(ke);
            return true;
        }
    }
    if (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut)
        viewport()->update();                         // show / hide the caret
    return QAbstractScrollArea::event(e);
}

void LargeFileView::keyPressEvent(QKeyEvent *e)
{
    const qint64 last = lineCount() - 1;
    const bool shift = e->modifiers() & Qt::ShiftModifier;
    const bool ctrl  = e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier);

    if (e->matches(QKeySequence::Copy))      { copySelection(); return; }
    if (e->matches(QKeySequence::Save))      { emit saveRequested(); return; }
    if (e->matches(QKeySequence::SelectAll)) { m_anchorLine = 0; m_cursorLine = last;
                                               viewport()->update(); return; }
    if (e->matches(QKeySequence::Undo))      { undo(); return; }
    if (e->matches(QKeySequence::Redo))      { redo(); return; }
    if (e->matches(QKeySequence::Cut)) {
        if (!isEditable()) return;
        copySelection();
        deleteSelectedLines();
        return;
    }
    if (e->matches(QKeySequence::Paste)) {
        insertText(QApplication::clipboard()->text());
        return;
    }

    // ── Editing keys ──
    if (isEditable()) {
        const QString cur = lineText(m_cursorLine);
        const int col = qMin<int>(m_cursorCol, cur.size());
        switch (e->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            insertText(QStringLiteral("\n"));
            return;
        case Qt::Key_Backspace:
            if (hasLineSelection()) { deleteSelectedLines(); return; }
            if (col > 0) {
                setLineText(m_cursorLine, cur.left(col - 1) + cur.mid(col), col - 1);
            } else if (m_cursorLine > 0) {                 // join with the previous line
                const QString prev = lineText(m_cursorLine - 1);
                commitReplace(m_cursorLine - 1, 2, { (prev + cur).toUtf8() },
                              m_cursorLine - 1, int(prev.size()));
            }
            return;
        case Qt::Key_Delete:
            if (hasLineSelection()) { deleteSelectedLines(); return; }
            if (col < cur.size()) {
                setLineText(m_cursorLine, cur.left(col) + cur.mid(col + 1), col);
            } else if (m_cursorLine < last) {              // join with the next line
                const QString next = lineText(m_cursorLine + 1);
                commitReplace(m_cursorLine, 2, { (cur + next).toUtf8() }, m_cursorLine, col);
            }
            return;
        case Qt::Key_Tab:
            insertText(QStringLiteral("\t"));
            return;
        default:
            break;
        }
        const QString text = e->text();
        if (!ctrl && !text.isEmpty() && text.at(0).isPrint()) {
            insertText(text);
            return;
        }
    }

    // ── Navigation ──
    qint64 line = m_cursorLine;
    int col = m_cursorCol;
    switch (e->key()) {
    case Qt::Key_Up:       line -= 1; break;
    case Qt::Key_Down:     line += 1; break;
    case Qt::Key_PageUp:   line -= visibleRows() - 1; break;
    case Qt::Key_PageDown: line += visibleRows() - 1; break;
    case Qt::Key_Left:
        if (col > 0) --col;
        else if (line > 0) { --line; col = int(lineText(line).size()); }
        break;
    case Qt::Key_Right:
        if (col < lineText(line).size()) ++col;
        else if (line < last) { ++line; col = 0; }
        break;
    case Qt::Key_Home: if (ctrl) line = 0;    col = 0; break;
    case Qt::Key_End:  if (ctrl) line = last; col = int(lineText(qBound<qint64>(0, line, last)).size()); break;
    default:           QAbstractScrollArea::keyPressEvent(e); return;
    }

    m_cursorLine = qBound<qint64>(0, line, last);
    m_cursorCol  = qMin<int>(col, lineText(m_cursorLine).size());
    if (!shift) m_anchorLine = m_cursorLine;
    ensureCursorVisible();
    viewport()->update();
    emit cursorMoved(m_cursorLine + 1);
}

// Accented / composed characters (dead keys, IMEs) arrive here, not as keys.
void LargeFileView::inputMethodEvent(QInputMethodEvent *e)
{
    if (!e->commitString().isEmpty()) insertText(e->commitString());
    e->accept();
}

void LargeFileView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return QAbstractScrollArea::mousePressEvent(e);
    const qint64 line = qMin<qint64>(lineCount() - 1,
                                     verticalScrollBar()->value() + e->position().y() / m_lineHeight);
    const int visual = qMax(0, int((e->position().x() - gutterWidth() - 4) / m_charWidth +
                                   horizontalScrollBar()->value() + 0.5));
    m_cursorLine = line;
    m_cursorCol  = columnForVisual(lineText(line), visual);
    if (!(e->modifiers() & Qt::ShiftModifier)) m_anchorLine = line;
    viewport()->update();
    emit cursorMoved(line + 1);
}

// Copies the selected lines (or the current line), capped in size.
void LargeFileView::copySelection()
{
    const qint64 from = qMin(m_anchorLine, m_cursorLine);
    const qint64 to   = qMax(m_anchorLine, m_cursorLine);
    QByteArray out;
    for (qint64 l = from; l <= to && out.size() < kMaxCopyBytes; ++l) {
        out += docLine(l);
        if (l < to) out += '\n';
    }
    if (from == to) out += '\n';                  // a whole line, like most editors
    QApplication::clipboard()->setText(QString::fromUtf8(out.left(kMaxCopyBytes)));
}

// ── Search ───────────────────────────────────────────────────────────────────

void LargeFileView::cancelFind()
{
    if (!m_finder) return;
    m_findCancel = true;
    m_finder->wait();
    delete m_finder;
    m_finder = nullptr;
}

void LargeFileView::findNext(const QString &text, Qt::CaseSensitivity cs)
{
    cancelFind();
    if (text.isEmpty()) return;
    if (!m_indexDone) { emit findResult(-1, false); return; }

    QByteArray needle = text.toUtf8();
    const bool icase = cs == Qt::CaseInsensitive;
    if (icase) needle = needle.toLower();
    m_findCancel = false;

    // The worker searches a snapshot of the document.
    const QVector<Piece> pieces = m_pieces;
    const QVector<qint64> prefix = m_prefix;
    const qint64 docLines = m_docLines;
    const qint64 startLine = (m_cursorLine + 1) % qMax<qint64>(1, docLines);

    m_finder = QThread::create([this, needle, icase, pieces, prefix, docLines, startLine] {
        const char *base = reinterpret_cast<const char *>(m_data);
        // Search document lines [from, to); returns a document line or -1.
        auto searchLines = [&](qint64 from, qint64 to) -> qint64 {
            for (int i = 0; i < pieces.size() && !m_findCancel; ++i) {
                const qint64 ps = prefix.at(i), pe = ps + pieces.at(i).count;
                if (pe <= from || ps >= to) continue;
                const qint64 lo = qMax(from, ps) - ps, hi = qMin(to, pe) - ps;
                const Piece &p = pieces.at(i);
                if (p.orig) {
                    const qint64 a = origLineStart(p.start + lo);
                    const qint64 b = origLineStart(p.start + hi);
                    const qint64 at = searchBytes(base, a, b, needle, icase, m_findCancel, m_data);
                    if (at >= 0) return ps + (origLineForOffset(at) - p.start);
                } else {
                    for (qint64 k = lo; k < hi; ++k) {
                        const QByteArray &l = p.lines.at(k);
                        if ((icase ? l.toLower() : l).contains(needle)) return ps + k;
                    }
                }
            }
            return -1;
        };
        qint64 line = searchLines(startLine, docLines);
        if (line < 0 && !m_findCancel) line = searchLines(0, startLine);
        if (m_findCancel) return;
        QMetaObject::invokeMethod(this, [this, line] {
            emit findResult(line < 0 ? -1 : line + 1, line >= 0);
        }, Qt::QueuedConnection);
    });
    m_finder->start(QThread::LowPriority);
}

// ═════════════════════════════════════════════════════════════════════════════
// LargeFileTab
// ═════════════════════════════════════════════════════════════════════════════

LargeFileTab::LargeFileTab(QWidget *parent)
    : QWidget(parent)
    , m_view(new LargeFileView(this))
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_info = new QLabel(this);
    m_info->setContentsMargins(8, 4, 8, 4);
    m_info->setTextFormat(Qt::RichText);

    // Find bar (hidden until Ctrl+F)
    m_findBar = new QWidget(this);
    auto *fl = new QHBoxLayout(m_findBar);
    fl->setContentsMargins(8, 4, 8, 4);
    m_findEdit = new QLineEdit(m_findBar);
    m_findEdit->setPlaceholderText(tr("Find in file…"));
    auto *matchCase = new QCheckBox(tr("Match case"), m_findBar);
    auto *next = new QPushButton(tr("Find Next"), m_findBar);
    m_findStatus = new QLabel(m_findBar);
    auto *close = new QToolButton(m_findBar);
    close->setText(QStringLiteral("✕"));
    close->setAutoRaise(true);
    fl->addWidget(m_findEdit, 1);
    fl->addWidget(matchCase);
    fl->addWidget(next);
    fl->addWidget(m_findStatus);
    fl->addWidget(close);
    m_findBar->hide();

    lay->addWidget(m_info);
    lay->addWidget(m_findBar);
    lay->addWidget(m_view, 1);

    auto find = [this, matchCase] {
        m_findStatus->setText(tr("Searching…"));
        m_view->findNext(m_findEdit->text(),
                         matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    };
    connect(m_findEdit, &QLineEdit::returnPressed, this, find);
    connect(next, &QPushButton::clicked, this, find);
    connect(close, &QToolButton::clicked, this, [this] {
        m_view->cancelFind();
        m_findBar->hide();
        m_view->setFocus();
    });
    connect(m_view, &LargeFileView::findResult, this, [this](qint64 line, bool found) {
        if (line < 0 && !found && m_view->isIndexing())
            m_findStatus->setText(tr("Wait for indexing to finish"));
        else if (!found)
            m_findStatus->setText(tr("Not found"));
        else {
            m_findStatus->setText(tr("Line %L1").arg(line));
            m_view->goToLine(line);
        }
    });

    connect(m_view, &LargeFileView::indexProgress, this, &LargeFileTab::updateInfo);
    connect(m_view, &LargeFileView::indexFinished, this, [this](qint64, qint64 ms) {
        m_indexMs = ms;
        updateInfo();
    });
    connect(m_view, &LargeFileView::cursorMoved, this, &LargeFileTab::updateInfo);
    connect(m_view, &LargeFileView::modificationChanged, this, [this](bool m) {
        updateInfo();
        emit modificationChanged(m);
    });
    connect(m_view, &LargeFileView::saveRequested, this, &LargeFileTab::save);
    connect(m_view, &LargeFileView::saveProgress, this, [this](int pct) {
        m_savePct = pct;
        updateInfo();
    });
    connect(m_view, &LargeFileView::saveFinished, this, [this](bool ok, const QString &err) {
        m_savePct = -1;
        m_lastError = ok ? QString() : err;
        updateInfo();
    });

    auto *findSc = new QShortcut(QKeySequence::Find, this, this, &LargeFileTab::showFind);
    findSc->setContext(Qt::WidgetWithChildrenShortcut);
    auto *nextSc = new QShortcut(QKeySequence::FindNext, this, this, find);
    nextSc->setContext(Qt::WidgetWithChildrenShortcut);
    auto *gotoSc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G), this, this,
                                 &LargeFileTab::promptGoToLine);
    gotoSc->setContext(Qt::WidgetWithChildrenShortcut);
    auto *escSc = new QShortcut(QKeySequence(Qt::Key_Escape), m_findBar, [close] { close->click(); });
    escSc->setContext(Qt::WidgetWithChildrenShortcut);
}

bool LargeFileTab::open(const QString &path, QString *error)
{
    const bool ok = m_view->open(path, error);
    updateInfo();
    m_view->setFocus();
    return ok;
}

void LargeFileTab::save()
{
    if (!m_view->isModified() || m_view->isSaving() || m_view->isIndexing()) return;
    m_savePct = 0;
    m_lastError.clear();
    updateInfo();
    m_view->save();
}

bool LargeFileTab::saveAndWait()
{
    if (!m_view->isModified()) return true;
    QProgressDialog dlg(tr("Saving %1…").arg(QFileInfo(filePath()).fileName()), QString(), 0, 100, this);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.setMinimumDuration(0);
    dlg.setValue(0);
    QEventLoop loop;
    bool ok = false;
    auto c1 = connect(m_view, &LargeFileView::saveProgress, &dlg, &QProgressDialog::setValue);
    auto c2 = connect(m_view, &LargeFileView::saveFinished, &loop,
                      [&](bool success, const QString &) { ok = success; loop.quit(); });
    save();
    if (m_view->isSaving()) loop.exec();
    disconnect(c1);
    disconnect(c2);
    return ok;
}

void LargeFileTab::updateInfo()
{
    const QLocale loc;
    QString state;
    if (m_view->isSaving()) {
        state = tr("<b>saving %1%</b>").arg(qMax(0, m_savePct));
    } else if (m_view->isIndexing()) {
        state = tr("indexing… %1 lines so far (read-only until done)").arg(loc.toString(m_view->lineCount()));
    } else {
        state = tr("%1 lines").arg(loc.toString(m_view->lineCount()));
        if (m_indexMs >= 0) state += tr(" (indexed in %1 s)").arg(m_indexMs / 1000.0, 0, 'f', 1);
    }
    if (m_view->isModified() && !m_view->isSaving()) state += tr(" · <b>modified</b>");
    if (!m_lastError.isEmpty())
        state += tr(" · <span style='color:#e06c75'>save failed: %1</span>").arg(m_lastError.toHtmlEscaped());

    m_info->setText(tr("<b>Large file mode</b> · %1 · %2 · Ln %3, Col %4 &nbsp;&nbsp;"
                       "<span style='color:gray'>Ctrl+S save · Ctrl+Z undo · Ctrl+F find · "
                       "Ctrl+G go to line</span>")
                        .arg(formatBytes(m_view->fileSize()), state,
                             loc.toString(m_view->currentLine()),
                             QString::number(m_view->currentColumn())));
}

void LargeFileTab::showFind()
{
    m_findBar->show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();
}

void LargeFileTab::promptGoToLine()
{
    bool ok = false;
    const qint64 max = m_view->lineCount();
    // QInputDialog::getInt is limited to int; 2 billion lines is plenty here.
    const int line = QInputDialog::getInt(this, tr("Go to Line"),
                                          tr("Line (1 – %L1):").arg(max),
                                          int(m_view->currentLine()), 1,
                                          int(qMin<qint64>(max, INT_MAX)), 1, &ok);
    if (ok) m_view->goToLine(line);
}
