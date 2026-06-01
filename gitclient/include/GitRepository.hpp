#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVector>
#include <memory>
#include <optional>

// Forward-declare libgit2 types to keep this header clean
struct git_repository;
struct git_commit;

namespace Kayte {

// ─── Data structs ────────────────────────────────────────────────────────────

struct CommitInfo {
    QString  oid;       ///< Full SHA-1 hex
    QString  shortOid;  ///< First 8 chars
    QString  message;
    QString  summary;   ///< First line of message
    QString  authorName;
    QString  authorEmail;
    QDateTime timestamp;
    QStringList parentOids;
};

struct BranchInfo {
    QString name;
    bool    isLocal  {true};
    bool    isHead   {false};
    QString trackingBranch;
    int     aheadCount  {0};
    int     behindCount {0};
};

struct StatusEntry {
    enum class State {
        Untracked, Modified, Added, Deleted, Renamed, Conflicted, Ignored
    };
    QString path;
    QString oldPath;  ///< non-empty for Renamed
    State   indexStatus  {State::Untracked};
    State   workdirStatus{State::Untracked};
};

struct DiffLine {
    enum class Origin { Context, Addition, Deletion, FileHeader, HunkHeader };
    Origin  origin;
    int     oldLineNo {-1};
    int     newLineNo {-1};
    QString content;
};

// ─── Repository class ────────────────────────────────────────────────────────

class GitRepository : public QObject {
    Q_OBJECT

public:
    explicit GitRepository(QObject *parent = nullptr);
    ~GitRepository() override;

    // Lifecycle
    bool open(const QString &path);
    bool init(const QString &path, bool bare = false);
    bool isOpen() const;
    QString workdir() const;
    QString path()    const;

    // Branches
    QVector<BranchInfo> branches(bool includeRemote = true) const;
    bool     checkout(const QString &branchName, bool createIfMissing = false);
    bool     createBranch(const QString &name, const QString &startPoint = {});
    bool     deleteBranch(const QString &name, bool force = false);
    QString  currentBranch() const;

    // Status
    QVector<StatusEntry> status() const;
    bool     stageFile(const QString &path);
    bool     unstageFile(const QString &path);
    bool     stageAll();
    bool     discardChanges(const QString &path);

    // Commits
    bool     commit(const QString &message,
                    const QString &authorName  = {},
                    const QString &authorEmail = {});
    QVector<CommitInfo> log(const QString &branch = {},
                            int maxCount = 200) const;
    std::optional<CommitInfo> commitInfo(const QString &oid) const;

    // Remotes
    QStringList remotes() const;
    bool     addRemote(const QString &name, const QString &url);
    bool     removeRemote(const QString &name);
    QString  remoteUrl(const QString &name) const;
    bool     fetch(const QString &remote = "origin");
    bool     pull(const QString &remote  = "origin",
                  const QString &branch  = {});
    bool     push(const QString &remote  = "origin",
                  const QString &branch  = {});

    // Diff
    QVector<DiffLine> diff(const QString &filePath = {}) const;
    QVector<DiffLine> diffCached(const QString &filePath = {}) const;

    // Tags
    QStringList tags() const;
    bool createTag(const QString &name, const QString &message = {},
                   const QString &ref = "HEAD");

    // Error
    QString lastError() const;

signals:
    void repositoryOpened(const QString &path);
    void branchChanged(const QString &newBranch);
    void statusChanged();
    void operationStarted(const QString &description);
    void operationFinished(bool success, const QString &message);
    void credentialsRequired(const QString &url,
                             const QString &usernameHint);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Kayte
