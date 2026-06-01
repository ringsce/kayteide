#pragma once
// SvnTypes.h – shared POD types and enumerations for the KaytEIDE SVN client
// C++17, Qt6

#include <QString>
#include <QDateTime>
#include <QList>
#include <QHash>
#include <QMetaType>   // required for Q_DECLARE_METATYPE

namespace Kayte::Svn {

// ── Status codes (mirrors svn_wc_status_kind_t) ─────────────────────────────
enum class StatusKind : int {
    None        = 0,
    Unversioned,
    Normal,
    Added,
    Missing,
    Deleted,
    Replaced,
    Modified,
    Merged,
    Conflicted,
    Ignored,
    Obstructed,
    External,
    Incomplete,
};

// ── Single file/directory status entry ──────────────────────────────────────
struct StatusEntry {
    QString    path;
    StatusKind textStatus   = StatusKind::None;
    StatusKind propStatus   = StatusKind::None;
    bool       versioned    = false;
    bool       switched     = false;
    bool       treeConflict = false;
    QString    author;
    qint64     revision     = -1;
    QDateTime  lastChanged;
};

// ── A single log entry ───────────────────────────────────────────────────────
struct LogEntry {
    qint64    revision;
    QString   author;
    QDateTime date;
    QString   message;
    QStringList changedPaths;   // e.g. { "M /trunk/src/foo.cpp", ... }
};

// ── Info about a working copy ────────────────────────────────────────────────
struct WcInfo {
    QString   wcPath;
    QString   url;
    QString   reposRootUrl;
    QString   reposUuid;
    qint64    revision    = -1;
    qint64    lastChanged = -1;
    QString   lastAuthor;
    QDateTime lastDate;
    QString   nodeKind;   // "file" | "dir"
    QString   schedule;   // "normal" | "add" | "delete" | "replace"
};

// ── Blame / annotate line ────────────────────────────────────────────────────
struct BlameLine {
    qint64  revision;
    QString author;
    QString line;
};

// ── Diff chunk ───────────────────────────────────────────────────────────────
struct DiffResult {
    QString path;
    QString unifiedDiff;   // raw unified diff text
};

// ── Commit result ────────────────────────────────────────────────────────────
struct CommitResult {
    bool    success     = false;
    qint64  newRevision = -1;
    QString error;
};

// ── Repository list entry ────────────────────────────────────────────────────
struct ListEntry {
    QString   name;
    bool      isDir    = false;
    qint64    size     = 0;
    qint64    revision = -1;
    QString   author;
    QDateTime date;
};

// ── Inline credential store ──────────────────────────────────────────────────
struct Credentials {
    QString username;
    QString password;
    bool    storePassword = false;
};

} // namespace Kayte::Svn

// ── QMetaType registration ───────────────────────────────────────────────────
// Declared here (in SvnTypes.h) so every translation unit that includes this
// header sees the same declaration exactly once.  Do NOT repeat these macros
// in SvnModel.h or anywhere else – that causes "explicit specialization after
// instantiation" errors when moc-generated files are compiled together.
Q_DECLARE_METATYPE(Kayte::Svn::StatusEntry)
Q_DECLARE_METATYPE(Kayte::Svn::StatusKind)
Q_DECLARE_METATYPE(Kayte::Svn::LogEntry)
Q_DECLARE_METATYPE(Kayte::Svn::WcInfo)
Q_DECLARE_METATYPE(Kayte::Svn::CommitResult)
Q_DECLARE_METATYPE(Kayte::Svn::BlameLine)
Q_DECLARE_METATYPE(Kayte::Svn::DiffResult)
