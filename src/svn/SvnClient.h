#pragma once
// SvnClient.h – Core async SVN client for KaytEIDE
// C++17 + Qt 6.  All heavy operations run on a background thread via
// QtConcurrent so the GUI never blocks.
//
// Usage:
//   auto *svn = new Kayte::Svn::SvnClient(this);
//   connect(svn, &SvnClient::statusReady, this, &MyPanel::onStatus);
//   svn->status("/path/to/wc");

#include "SvnTypes.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFuture>
#include <QFutureWatcher>
#include <memory>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Kayte::Svn {

class SvnClientPrivate;

class SvnClient : public QObject
{
    Q_OBJECT

public:
    explicit SvnClient(QObject *parent = nullptr);
    ~SvnClient() override;

    // ── Configuration ────────────────────────────────────────────────────────
    void setCredentials(const Credentials &creds);
    void setSvnExecutable(const QString &path);   // default: "svn" (from PATH)
    [[nodiscard]] QString svnExecutable() const;

    // ── Working copy operations (async) ──────────────────────────────────────

    /// svn status --xml <wcPath>
    void status(const QString &wcPath, bool showUnversioned = true);

    /// svn update [paths]
    void update(const QStringList &paths);

    /// svn commit -m <msg> [paths]
    void commit(const QStringList &paths, const QString &message);

    /// svn add <paths>
    void add(const QStringList &paths);

    /// svn revert --recursive <paths>
    void revert(const QStringList &paths);

    /// svn delete <paths>
    void remove(const QStringList &paths, bool keepLocal = false);

    /// svn copy <src> <dst>
    void copy(const QString &src, const QString &dst);

    /// svn move <src> <dst>
    void move(const QString &src, const QString &dst);

    /// svn mkdir <url|path>
    void mkdir(const QString &path, const QString &message = {});

    /// svn log --xml --limit <limit> <path>
    void log(const QString &path, int limit = 50, qint64 startRev = -1, qint64 endRev = -1);

    /// svn diff [--revision OLD:NEW] <path>
    void diff(const QString &path, qint64 rev1 = -1, qint64 rev2 = -1);

    /// svn blame --xml <path>
    void blame(const QString &path, qint64 startRev = 1, qint64 endRev = -1);

    /// svn info --xml <path>
    void info(const QString &path);

    /// svn list --xml <url>
    void list(const QString &url, bool recursive = false);

    /// svn checkout <url> <localDir>
    void checkout(const QString &url, const QString &localDir, qint64 rev = -1);

    /// svn export <url|path> <destination>
    void exportWc(const QString &src, const QString &dst);

    /// svn cleanup <path>
    void cleanup(const QString &path);

    /// svn resolve --accept=<choice> <paths>
    enum class ResolveChoice { Postpone, Mine, Theirs, MineFull, TheirsFull, Merged };
    void resolve(const QStringList &paths, ResolveChoice choice);

    /// svn merge <src> <target>
    void merge(const QString &src, const QString &target, qint64 rev1 = -1, qint64 rev2 = -1);

    /// svn switch <url> <path>
    void switchUrl(const QString &path, const QString &newUrl);

    /// svn propset <propName> <value> <path>
    void propSet(const QString &path, const QString &propName, const QString &value);

    /// svn propget <propName> <path>
    void propGet(const QString &path, const QString &propName);

    // ── Synchronous helpers (use with care – only from non-GUI threads) ───────
    [[nodiscard]] WcInfo infoSync(const QString &path) const;
    [[nodiscard]] QList<StatusEntry> statusSync(const QString &path,
                                                bool showUnversioned = true) const;

Q_SIGNALS:
    // Emitted when async operations complete
    void statusReady(const QList<Kayte::Svn::StatusEntry> &entries);
    void updateFinished(const QStringList &updatedPaths, qint64 newRevision);
    void commitFinished(const Kayte::Svn::CommitResult &result);
    void logReady(const QList<Kayte::Svn::LogEntry> &entries);
    void diffReady(const Kayte::Svn::DiffResult &result);
    void blameReady(const QList<Kayte::Svn::BlameLine> &lines);
    void infoReady(const Kayte::Svn::WcInfo &info);
    void listReady(const QList<Kayte::Svn::ListEntry> &entries);
    void checkoutProgress(int percent, const QString &currentPath);
    void checkoutFinished(bool success, const QString &error);
    void operationFinished(const QString &operation, bool success, const QString &output);

    // General progress/error
    void outputLine(const QString &line);
    void errorOccurred(const QString &operation, const QString &message);

private:
    std::unique_ptr<SvnClientPrivate> d;
};

} // namespace Kayte::Svn
