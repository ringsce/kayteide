#include "GitRepository.hpp"

#include <git2.h>
#include <git2/remote.h>
#include <git2/credential.h>
#include <git2/transport.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <functional>

namespace Kayte {

// ─── libgit2 RAII helpers ────────────────────────────────────────────────────

template<typename T, void(*Deleter)(T*)>
struct GitPtr {
    T *ptr {nullptr};
    explicit GitPtr(T *p = nullptr) : ptr(p) {}
    ~GitPtr() { if (ptr) Deleter(ptr); }
    GitPtr(const GitPtr&) = delete;
    GitPtr& operator=(const GitPtr&) = delete;
    GitPtr(GitPtr &&o) noexcept : ptr(std::exchange(o.ptr, nullptr)) {}
    GitPtr& operator=(GitPtr &&o) noexcept {
        if (this != std::addressof(o)) {
            if (ptr) Deleter(ptr);
            ptr = std::exchange(o.ptr, nullptr);
        }
        return *this;
    }
    T**  operator&()    { return &ptr; }
    T*   get()    const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }
};

using RepoPtr    = GitPtr<git_repository,    git_repository_free>;
using CommitPtr  = GitPtr<git_commit,         git_commit_free>;
using BranchIter = GitPtr<git_branch_iterator,git_branch_iterator_free>;
using StatusList = GitPtr<git_status_list,    git_status_list_free>;
using DiffPtr    = GitPtr<git_diff,           git_diff_free>;
using TreePtr    = GitPtr<git_tree,           git_tree_free>;
using IndexPtr   = GitPtr<git_index,          git_index_free>;
using RemotePtr  = GitPtr<git_remote,         git_remote_free>;
using RevwalkPtr = GitPtr<git_revwalk,        git_revwalk_free>;
using RefPtr     = GitPtr<git_reference,      git_reference_free>;
using TagIter    = GitPtr<git_strarray,       git_strarray_dispose>;

// ─── Pimpl ───────────────────────────────────────────────────────────────────

struct GitRepository::Impl {
    RepoPtr  repo;
    QString  lastError;
    QString  pendingCredUser;
    QString  pendingCredPass;
    bool     credentialsProvided {false};
    QMutex   credMutex;

    // Called by libgit2 when credentials are needed
    static int credentialCallback(git_credential **out,
                                   const char *url,
                                   const char *username_from_url,
                                   unsigned int /*allowed_types*/,
                                   void *payload)
    {
        auto *self = static_cast<Impl*>(payload);
        QMutexLocker lk(&self->credMutex);
        if (!self->credentialsProvided) return GIT_EAUTH;

        return git_credential_userpass_plaintext_new(
            out,
            self->pendingCredUser.toUtf8().constData(),
            self->pendingCredPass.toUtf8().constData());
    }

    // Fill git_remote_callbacks with our credential hook
    git_remote_callbacks makeCallbacks() {
        git_remote_callbacks cb = GIT_REMOTE_CALLBACKS_INIT;
        cb.credentials = &Impl::credentialCallback;
        cb.payload     = this;
        return cb;
    }

    void setError(int rc) {
        const git_error *e = git_error_last();
        lastError = e ? QString::fromUtf8(e->message)
                      : QString("libgit2 error code %1").arg(rc);
    }

    bool check(int rc) {
        if (rc >= 0) return true;
        setError(rc);
        return false;
    }
};

// ─── Global libgit2 init (once per process) ──────────────────────────────────

struct LibGit2Init {
    LibGit2Init()  { git_libgit2_init();     }
    ~LibGit2Init() { git_libgit2_shutdown(); }
};
static LibGit2Init s_libgit2;

// ─── GitRepository ───────────────────────────────────────────────────────────

GitRepository::GitRepository(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>()) {}

GitRepository::~GitRepository() = default;

bool GitRepository::open(const QString &path) {
    RepoPtr r;
    int rc = git_repository_open(&r.ptr, path.toUtf8().constData());
    if (!d->check(rc)) return false;
    d->repo = std::move(r);
    emit repositoryOpened(path);
    return true;
}

bool GitRepository::init(const QString &path, bool bare) {
    RepoPtr r;
    int rc = git_repository_init(&r.ptr, path.toUtf8().constData(), bare ? 1 : 0);
    if (!d->check(rc)) return false;
    d->repo = std::move(r);
    emit repositoryOpened(path);
    return true;
}

bool GitRepository::isOpen() const { return d->repo.ptr != nullptr; }

QString GitRepository::workdir() const {
    if (!isOpen()) return {};
    const char *wd = git_repository_workdir(d->repo.get());
    return wd ? QString::fromUtf8(wd) : QString{};
}

QString GitRepository::path() const {
    if (!isOpen()) return {};
    return QString::fromUtf8(git_repository_path(d->repo.get()));
}

QString GitRepository::lastError() const { return d->lastError; }

// ─── Branches ────────────────────────────────────────────────────────────────

QString GitRepository::currentBranch() const {
    if (!isOpen()) return {};
    RefPtr head;
    if (git_repository_head(&head.ptr, d->repo.get()) < 0) return "(detached)";
    const char *name = nullptr;
    git_branch_name(&name, head.get());
    return name ? QString::fromUtf8(name) : QString("(detached)");
}

QVector<BranchInfo> GitRepository::branches(bool includeRemote) const {
    QVector<BranchInfo> result;
    if (!isOpen()) return result;

    const QString head = currentBranch();
    git_branch_t flags = includeRemote
        ? static_cast<git_branch_t>(GIT_BRANCH_LOCAL | GIT_BRANCH_REMOTE)
        : GIT_BRANCH_LOCAL;

    BranchIter iter;
    if (git_branch_iterator_new(&iter.ptr, d->repo.get(), flags) < 0)
        return result;

    git_reference *ref = nullptr;
    git_branch_t   type{};
    while (git_branch_next(&ref, &type, iter.get()) == 0) {
        RefPtr rp(ref);
        const char *name = nullptr;
        git_branch_name(&name, rp.get());
        if (!name) continue;

        BranchInfo b;
        b.name    = QString::fromUtf8(name);
        b.isLocal = (type == GIT_BRANCH_LOCAL);
        b.isHead  = (b.name == head);

        // upstream tracking info
        if (b.isLocal) {
            RefPtr upstream;
            if (git_branch_upstream(&upstream.ptr, rp.get()) == 0) {
                const char *uname = nullptr;
                git_branch_name(&uname, upstream.get());
                b.trackingBranch = uname ? QString::fromUtf8(uname) : QString{};

                git_oid local_oid{}, upstream_oid{};
                git_reference_name_to_id(&local_oid,    d->repo.get(), git_reference_name(rp.get()));
                git_reference_name_to_id(&upstream_oid, d->repo.get(), git_reference_name(upstream.get()));
                size_t ahead = 0, behind = 0;
                git_graph_ahead_behind(&ahead, &behind, d->repo.get(), &local_oid, &upstream_oid);
                b.aheadCount  = static_cast<int>(ahead);
                b.behindCount = static_cast<int>(behind);
            }
        }
        result.push_back(std::move(b));
    }
    return result;
}

bool GitRepository::checkout(const QString &branchName, bool createIfMissing) {
    if (!isOpen()) return false;
    emit operationStarted(QString("Checking out '%1'").arg(branchName));

    // Try local branch first
    RefPtr ref;
    QByteArray fullName = ("refs/heads/" + branchName).toUtf8();
    int rc = git_reference_lookup(&ref.ptr, d->repo.get(), fullName.constData());

    if (rc < 0 && createIfMissing) {
        if (!createBranch(branchName)) {
            emit operationFinished(false, d->lastError);
            return false;
        }
        rc = git_reference_lookup(&ref.ptr, d->repo.get(), fullName.constData());
    }
    if (!d->check(rc)) { emit operationFinished(false, d->lastError); return false; }

    CommitPtr commit;
    git_oid oid;
    git_reference_name_to_id(&oid, d->repo.get(), fullName.constData());
    if (!d->check(git_commit_lookup(&commit.ptr, d->repo.get(), &oid))) {
        emit operationFinished(false, d->lastError); return false;
    }

    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    rc = git_checkout_tree(d->repo.get(),
                           reinterpret_cast<const git_object*>(commit.get()), &opts);
    if (!d->check(rc)) { emit operationFinished(false, d->lastError); return false; }

    rc = git_repository_set_head(d->repo.get(), fullName.constData());
    if (!d->check(rc)) { emit operationFinished(false, d->lastError); return false; }

    emit branchChanged(branchName);
    emit operationFinished(true, QString("Checked out '%1'").arg(branchName));
    return true;
}

bool GitRepository::createBranch(const QString &name, const QString &startPoint) {
    if (!isOpen()) return false;
    CommitPtr commit;
    git_oid oid;
    const QByteArray ref = startPoint.isEmpty()
                           ? QByteArray("HEAD")
                           : startPoint.toUtf8();
    if (!d->check(git_reference_name_to_id(&oid, d->repo.get(), ref.constData())))
        return false;
    if (!d->check(git_commit_lookup(&commit.ptr, d->repo.get(), &oid)))
        return false;
    RefPtr branch;
    return d->check(git_branch_create(&branch.ptr, d->repo.get(),
                                       name.toUtf8().constData(), commit.get(), 0));
}

bool GitRepository::deleteBranch(const QString &name, bool force) {
    if (!isOpen()) return false;
    RefPtr ref;
    if (!d->check(git_branch_lookup(&ref.ptr, d->repo.get(),
                                    name.toUtf8().constData(), GIT_BRANCH_LOCAL)))
        return false;
    return d->check(git_branch_delete(ref.get()));
    Q_UNUSED(force);
}

// ─── Status & Staging ────────────────────────────────────────────────────────

QVector<StatusEntry> GitRepository::status() const {
    QVector<StatusEntry> result;
    if (!isOpen()) return result;

    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show  = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED
               | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    StatusList list;
    if (!d->check(git_status_list_new(&list.ptr, d->repo.get(), &opts)))
        return result;

    auto mapState = [](unsigned int flags, bool index) -> StatusEntry::State {
        if (index) {
            if (flags & GIT_STATUS_INDEX_NEW)      return StatusEntry::State::Added;
            if (flags & GIT_STATUS_INDEX_MODIFIED) return StatusEntry::State::Modified;
            if (flags & GIT_STATUS_INDEX_DELETED)  return StatusEntry::State::Deleted;
            if (flags & GIT_STATUS_INDEX_RENAMED)  return StatusEntry::State::Renamed;
        } else {
            if (flags & GIT_STATUS_WT_NEW)         return StatusEntry::State::Untracked;
            if (flags & GIT_STATUS_WT_MODIFIED)    return StatusEntry::State::Modified;
            if (flags & GIT_STATUS_WT_DELETED)     return StatusEntry::State::Deleted;
            if (flags & GIT_STATUS_WT_RENAMED)     return StatusEntry::State::Renamed;
            if (flags & GIT_STATUS_IGNORED)        return StatusEntry::State::Ignored;
            if (flags & GIT_STATUS_CONFLICTED)     return StatusEntry::State::Conflicted;
        }
        return StatusEntry::State::Untracked;
    };

    const size_t count = git_status_list_entrycount(list.get());
    result.reserve(static_cast<int>(count));
    for (size_t i = 0; i < count; ++i) {
        const git_status_entry *entry = git_status_byindex(list.get(), i);
        StatusEntry se;
        const char *path = entry->index_to_workdir ? entry->index_to_workdir->new_file.path
                                                   : entry->head_to_index   ->new_file.path;
        se.path = QString::fromUtf8(path);
        se.indexStatus   = mapState(entry->status, true);
        se.workdirStatus = mapState(entry->status, false);
        result.push_back(std::move(se));
    }
    return result;
}

bool GitRepository::stageFile(const QString &path) {
    if (!isOpen()) return false;
    IndexPtr idx;
    if (!d->check(git_repository_index(&idx.ptr, d->repo.get()))) return false;
    if (!d->check(git_index_add_bypath(idx.get(), path.toUtf8().constData()))) return false;
    bool ok = d->check(git_index_write(idx.get()));
    if (ok) emit statusChanged();
    return ok;
}

bool GitRepository::unstageFile(const QString &path) {
    if (!isOpen()) return false;
    git_oid head_oid{};
    git_reference_name_to_id(&head_oid, d->repo.get(), "HEAD");
    CommitPtr head_commit;
    git_commit_lookup(&head_commit.ptr, d->repo.get(), &head_oid);
    const git_commit *parents[] = { head_commit.get() };
    int rc = git_reset_default(d->repo.get(),
                               reinterpret_cast<const git_object*>(head_commit.get()),
                               nullptr);
    // Fine-grained: use strarray for the single path
    git_strarray pathspecs;
    const char *cpath = path.toUtf8().constData();
    pathspecs.strings = const_cast<char**>(&cpath);
    pathspecs.count   = 1;
    rc = git_reset_default(d->repo.get(),
                           reinterpret_cast<const git_object*>(head_commit.get()),
                           &pathspecs);
    bool ok = d->check(rc);
    if (ok) emit statusChanged();
    return ok;
}

bool GitRepository::stageAll() {
    if (!isOpen()) return false;
    IndexPtr idx;
    if (!d->check(git_repository_index(&idx.ptr, d->repo.get()))) return false;
    git_strarray paths = { nullptr, 0 }; // null = all
    if (!d->check(git_index_add_all(idx.get(), &paths,
                                    GIT_INDEX_ADD_DEFAULT, nullptr, nullptr))) return false;
    bool ok = d->check(git_index_write(idx.get()));
    if (ok) emit statusChanged();
    return ok;
}

bool GitRepository::discardChanges(const QString &path) {
    if (!isOpen()) return false;
    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    const char *cpaths[] = { path.toUtf8().constData() };
    git_strarray ps { const_cast<char**>(cpaths), 1 };
    opts.paths = ps;
    bool ok = d->check(git_checkout_head(d->repo.get(), &opts));
    if (ok) emit statusChanged();
    return ok;
}

// ─── Commits ─────────────────────────────────────────────────────────────────

bool GitRepository::commit(const QString &message,
                            const QString &authorName,
                            const QString &authorEmail) {
    if (!isOpen()) return false;
    emit operationStarted("Committing…");

    // Retrieve default sig if none supplied
    git_signature *sig = nullptr;
    if (authorName.isEmpty()) {
        if (!d->check(git_signature_default(&sig, d->repo.get()))) {
            emit operationFinished(false, d->lastError); return false;
        }
    } else {
        if (!d->check(git_signature_now(&sig, authorName.toUtf8().constData(),
                                        authorEmail.toUtf8().constData()))) {
            emit operationFinished(false, d->lastError); return false;
        }
    }
    auto sigGuard = std::unique_ptr<git_signature, decltype(&git_signature_free)>{
        sig, git_signature_free
    };

    IndexPtr idx;
    if (!d->check(git_repository_index(&idx.ptr, d->repo.get()))) {
        emit operationFinished(false, d->lastError); return false;
    }
    git_oid tree_oid{};
    if (!d->check(git_index_write_tree(&tree_oid, idx.get()))) {
        emit operationFinished(false, d->lastError); return false;
    }
    TreePtr tree;
    if (!d->check(git_tree_lookup(&tree.ptr, d->repo.get(), &tree_oid))) {
        emit operationFinished(false, d->lastError); return false;
    }

    // Parent: HEAD (may not exist for first commit)
    git_oid parent_oid{};
    CommitPtr parent;
    bool hasParent = (git_reference_name_to_id(&parent_oid, d->repo.get(), "HEAD") == 0);
    if (hasParent)
        git_commit_lookup(&parent.ptr, d->repo.get(), &parent_oid);

    const git_commit *parents[] = { parent.get() };
    git_oid commit_oid{};
    int rc = git_commit_create(&commit_oid, d->repo.get(), "HEAD",
                               sig, sig,
                               "UTF-8", message.toUtf8().constData(),
                               tree.get(),
                               hasParent ? 1u : 0u,
                               hasParent ? parents : nullptr);
    if (!d->check(rc)) { emit operationFinished(false, d->lastError); return false; }

    char oidStr[GIT_OID_HEXSZ + 1] = {};
    git_oid_tostr(oidStr, sizeof(oidStr), &commit_oid);
    emit statusChanged();
    emit operationFinished(true, QString("Committed %1").arg(oidStr).left(12));
    return true;
}

QVector<CommitInfo> GitRepository::log(const QString &branch, int maxCount) const {
    QVector<CommitInfo> result;
    if (!isOpen()) return result;

    RevwalkPtr walk;
    if (!d->check(git_revwalk_new(&walk.ptr, d->repo.get()))) return result;
    git_revwalk_sorting(walk.get(), GIT_SORT_TIME);

    if (branch.isEmpty()) {
        git_revwalk_push_head(walk.get());
    } else {
        git_oid oid{};
        QByteArray ref = ("refs/heads/" + branch).toUtf8();
        if (git_reference_name_to_id(&oid, d->repo.get(), ref.constData()) < 0)
            return result;
        git_revwalk_push(walk.get(), &oid);
    }

    git_oid oid{};
    int count = 0;
    while (git_revwalk_next(&oid, walk.get()) == 0 && count < maxCount) {
        CommitPtr c;
        if (git_commit_lookup(&c.ptr, d->repo.get(), &oid) < 0) continue;

        CommitInfo ci;
        char buf[GIT_OID_HEXSZ + 1] = {};
        git_oid_tostr(buf, sizeof(buf), &oid);
        ci.oid      = QString::fromLatin1(buf);
        ci.shortOid = ci.oid.left(8);
        ci.message  = QString::fromUtf8(git_commit_message(c.get()));
        ci.summary  = ci.message.split('\n').first().trimmed();

        const git_signature *auth = git_commit_author(c.get());
        ci.authorName  = QString::fromUtf8(auth->name);
        ci.authorEmail = QString::fromUtf8(auth->email);
        ci.timestamp   = QDateTime::fromSecsSinceEpoch(
                             auth->when.time,
                             QTimeZone::fromSecondsAheadOfUtc(auth->when.offset * 60));

        for (unsigned p = 0; p < git_commit_parentcount(c.get()); ++p) {
            char pbuf[GIT_OID_HEXSZ + 1] = {};
            git_oid_tostr(pbuf, sizeof(pbuf), git_commit_parent_id(c.get(), p));
            ci.parentOids << QString::fromLatin1(pbuf);
        }
        result.push_back(std::move(ci));
        ++count;
    }
    return result;
}

// ─── Remotes ─────────────────────────────────────────────────────────────────

QStringList GitRepository::remotes() const {
    QStringList list;
    if (!isOpen()) return list;
    git_strarray arr{};
    if (git_remote_list(&arr, d->repo.get()) == 0) {
        for (size_t i = 0; i < arr.count; ++i)
            list << QString::fromUtf8(arr.strings[i]);
        git_strarray_dispose(&arr);
    }
    return list;
}

bool GitRepository::addRemote(const QString &name, const QString &url) {
    if (!isOpen()) return false;
    RemotePtr remote;
    return d->check(git_remote_create(&remote.ptr, d->repo.get(),
                                       name.toUtf8().constData(),
                                       url.toUtf8().constData()));
}

bool GitRepository::removeRemote(const QString &name) {
    if (!isOpen()) return false;
    return d->check(git_remote_delete(d->repo.get(), name.toUtf8().constData()));
}

QString GitRepository::remoteUrl(const QString &name) const {
    if (!isOpen()) return {};
    RemotePtr remote;
    if (git_remote_lookup(&remote.ptr, d->repo.get(), name.toUtf8().constData()) < 0)
        return {};
    return QString::fromUtf8(git_remote_url(remote.get()));
}

bool GitRepository::fetch(const QString &remote) {
    if (!isOpen()) return false;
    emit operationStarted(QString("Fetching '%1'…").arg(remote));

    RemotePtr r;
    if (!d->check(git_remote_lookup(&r.ptr, d->repo.get(), remote.toUtf8().constData()))) {
        emit operationFinished(false, d->lastError); return false;
    }
    git_remote_callbacks cb = d->makeCallbacks();
    git_fetch_options opts  = GIT_FETCH_OPTIONS_INIT;
    opts.callbacks          = cb;
    bool ok = d->check(git_remote_fetch(r.get(), nullptr, &opts, nullptr));
    emit operationFinished(ok, ok ? "Fetch complete" : d->lastError);
    return ok;
}

bool GitRepository::push(const QString &remote, const QString &branch) {
    if (!isOpen()) return false;
    const QString br = branch.isEmpty() ? currentBranch() : branch;
    emit operationStarted(QString("Pushing '%1' → '%2'…").arg(br, remote));

    RemotePtr r;
    if (!d->check(git_remote_lookup(&r.ptr, d->repo.get(), remote.toUtf8().constData()))) {
        emit operationFinished(false, d->lastError); return false;
    }

    QByteArray refspec = (QString("refs/heads/%1:refs/heads/%1").arg(br)).toUtf8();
    const char *refspecs[] = { refspec.constData() };
    git_strarray refs { const_cast<char**>(refspecs), 1 };

    git_remote_callbacks cb = d->makeCallbacks();
    git_push_options opts   = GIT_PUSH_OPTIONS_INIT;
    opts.callbacks          = cb;
    bool ok = d->check(git_remote_push(r.get(), &refs, &opts));
    emit operationFinished(ok, ok ? "Push complete" : d->lastError);
    return ok;
}

bool GitRepository::pull(const QString &remote, const QString &branch) {
    // pull = fetch + merge
    if (!fetch(remote)) return false;
    const QString br  = branch.isEmpty() ? currentBranch() : branch;
    const QString ref = QString("refs/remotes/%1/%2").arg(remote, br);

    git_oid remote_oid{};
    if (!d->check(git_reference_name_to_id(&remote_oid, d->repo.get(),
                                            ref.toUtf8().constData()))) return false;

    git_annotated_commit *ancommit = nullptr;
    git_annotated_commit_from_fetchhead(&ancommit, d->repo.get(),
                                        br.toUtf8().constData(),
                                        remote.toUtf8().constData(),
                                        &remote_oid);
    auto acGuard = std::unique_ptr<git_annotated_commit,
                                    decltype(&git_annotated_commit_free)>{
        ancommit, git_annotated_commit_free
    };

    git_merge_options mopts   = GIT_MERGE_OPTIONS_INIT;
    git_checkout_options copts = GIT_CHECKOUT_OPTIONS_INIT;
    copts.checkout_strategy   = GIT_CHECKOUT_SAFE;

    const git_annotated_commit *commits[] = { ancommit };
    bool ok = d->check(git_merge(d->repo.get(), commits, 1, &mopts, &copts));
    if (ok) emit statusChanged();
    emit operationFinished(ok, ok ? "Pull complete" : d->lastError);
    return ok;
}

// ─── Diff ────────────────────────────────────────────────────────────────────

static QVector<DiffLine> parseDiff(git_diff *diff) {
    QVector<DiffLine> lines;
    git_diff_print(diff, GIT_DIFF_FORMAT_PATCH,
        [](const git_diff_delta */*d*/, const git_diff_hunk */*h*/,
           const git_diff_line *line, void *payload) -> int
        {
            auto *out = static_cast<QVector<DiffLine>*>(payload);
            DiffLine dl;
            switch (line->origin) {
            case GIT_DIFF_LINE_CONTEXT:      dl.origin = DiffLine::Origin::Context;    break;
            case GIT_DIFF_LINE_ADDITION:     dl.origin = DiffLine::Origin::Addition;   break;
            case GIT_DIFF_LINE_DELETION:     dl.origin = DiffLine::Origin::Deletion;   break;
            case GIT_DIFF_LINE_FILE_HDR:     dl.origin = DiffLine::Origin::FileHeader; break;
            case GIT_DIFF_LINE_HUNK_HDR:     dl.origin = DiffLine::Origin::HunkHeader; break;
            default:                         dl.origin = DiffLine::Origin::Context;    break;
            }
            dl.oldLineNo = line->old_lineno;
            dl.newLineNo = line->new_lineno;
            dl.content   = QString::fromUtf8(line->content,
                               static_cast<int>(line->content_len));
            out->push_back(std::move(dl));
            return 0;
        }, &lines);
    return lines;
}

QVector<DiffLine> GitRepository::diff(const QString &filePath) const {
    if (!isOpen()) return {};
    DiffPtr df;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    if (!filePath.isEmpty()) {
        const char *cp = filePath.toUtf8().constData();
        opts.pathspec.strings = const_cast<char**>(&cp);
        opts.pathspec.count   = 1;
    }
    if (git_diff_index_to_workdir(&df.ptr, d->repo.get(), nullptr, &opts) < 0)
        return {};
    return parseDiff(df.get());
}

QVector<DiffLine> GitRepository::diffCached(const QString &filePath) const {
    if (!isOpen()) return {};
    git_oid head_oid{};
    git_reference_name_to_id(&head_oid, d->repo.get(), "HEAD");
    CommitPtr head;
    git_commit_lookup(&head.ptr, d->repo.get(), &head_oid);
    TreePtr headTree;
    git_commit_tree(&headTree.ptr, head.get());

    DiffPtr df;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    if (!filePath.isEmpty()) {
        const char *cp = filePath.toUtf8().constData();
        opts.pathspec.strings = const_cast<char**>(&cp);
        opts.pathspec.count   = 1;
    }
    if (git_diff_tree_to_index(&df.ptr, d->repo.get(), headTree.get(), nullptr, &opts) < 0)
        return {};
    return parseDiff(df.get());
}

// ─── Tags ────────────────────────────────────────────────────────────────────

QStringList GitRepository::tags() const {
    QStringList list;
    if (!isOpen()) return list;
    git_strarray arr{};
    if (git_tag_list(&arr, d->repo.get()) == 0) {
        for (size_t i = 0; i < arr.count; ++i)
            list << QString::fromUtf8(arr.strings[i]);
        git_strarray_dispose(&arr);
    }
    return list;
}

bool GitRepository::createTag(const QString &name, const QString &message,
                               const QString &ref) {
    if (!isOpen()) return false;
    git_oid oid{};
    git_reference_name_to_id(&oid, d->repo.get(), ref.toUtf8().constData());
    git_object *obj = nullptr;
    git_object_lookup(&obj, d->repo.get(), &oid, GIT_OBJECT_ANY);
    auto objGuard = std::unique_ptr<git_object,decltype(&git_object_free)>{obj,git_object_free};

    git_oid tag_oid{};
    if (message.isEmpty())
        return d->check(git_tag_create_lightweight(&tag_oid, d->repo.get(),
                                                    name.toUtf8().constData(), obj, 0));
    git_signature *sig = nullptr;
    git_signature_default(&sig, d->repo.get());
    auto sigG = std::unique_ptr<git_signature,decltype(&git_signature_free)>{sig,git_signature_free};
    return d->check(git_tag_create(&tag_oid, d->repo.get(),
                                   name.toUtf8().constData(), obj, sig,
                                   message.toUtf8().constData(), 0));
}

std::optional<CommitInfo> GitRepository::commitInfo(const QString &oid) const {
    if (!isOpen()) return std::nullopt;
    git_oid git_oid_val{};
    if (git_oid_fromstr(&git_oid_val, oid.toUtf8().constData()) < 0) return std::nullopt;
    CommitPtr c;
    if (git_commit_lookup(&c.ptr, d->repo.get(), &git_oid_val) < 0) return std::nullopt;

    CommitInfo ci;
    ci.oid      = oid;
    ci.shortOid = oid.left(8);
    ci.message  = QString::fromUtf8(git_commit_message(c.get()));
    ci.summary  = ci.message.split('\n').first().trimmed();
    const git_signature *auth = git_commit_author(c.get());
    ci.authorName  = QString::fromUtf8(auth->name);
    ci.authorEmail = QString::fromUtf8(auth->email);
    ci.timestamp   = QDateTime::fromSecsSinceEpoch(auth->when.time);
    return ci;
}

} // namespace Kayte
