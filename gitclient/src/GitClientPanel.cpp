#include "GitClientPanel.hpp"
#include "GitCredentialDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QListWidgetItem>
#include <QIcon>
#include <QApplication>
#include <QtConcurrent>

namespace Kayte {

// ─── Construction ─────────────────────────────────────────────────────────────

GitClientPanel::GitClientPanel(QWidget *parent) : QWidget(parent) {
    m_repo     = new GitRepository(this);
    m_logModel = new GitLogModel(m_repo, this);
    buildUi();
    setupConnections();
}

bool GitClientPanel::openRepository(const QString &path) {
    bool ok = m_repo->open(path);
    if (!ok) QMessageBox::warning(this, "Git", m_repo->lastError());
    return ok;
}

GitRepository *GitClientPanel::repository() const { return m_repo; }

// ─── UI construction ──────────────────────────────────────────────────────────

void GitClientPanel::buildUi() {
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(2);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(18, 18));

    auto *actOpen   = m_toolbar->addAction("📂 Open");
    auto *actFetch  = m_toolbar->addAction("⬇ Fetch");
    auto *actPull   = m_toolbar->addAction("⬇⬇ Pull");
    auto *actPush   = m_toolbar->addAction("⬆ Push");
    m_toolbar->addSeparator();
    auto *actCommit = m_toolbar->addAction("✔ Commit");
    m_toolbar->addSeparator();

    m_branchCombo = new QComboBox(this);
    m_branchCombo->setMinimumWidth(150);
    m_toolbar->addWidget(new QLabel("  Branch: "));
    m_toolbar->addWidget(m_branchCombo);

    rootLayout->addWidget(m_toolbar);

    // ── Main splitter (horizontal: branch mgr | right pane) ──────────────────
    auto *hSplitter = new QSplitter(Qt::Horizontal, this);

    m_branchMgr = new GitBranchManager(m_repo, this);
    m_branchMgr->setMaximumWidth(240);
    hSplitter->addWidget(m_branchMgr);

    // ── Right vertical splitter: log | status | diff ──────────────────────
    auto *vSplitter = new QSplitter(Qt::Vertical, this);

    m_logView = new QTableView(this);
    m_logView->setModel(m_logModel);
    m_logView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_logView->horizontalHeader()->setSectionResizeMode(GitLogModel::ColSummary,
                                                         QHeaderView::Stretch);
    m_logView->horizontalHeader()->setSectionResizeMode(GitLogModel::ColGraph,
                                                         QHeaderView::Fixed);
    m_logView->setColumnWidth(GitLogModel::ColGraph,    30);
    m_logView->setColumnWidth(GitLogModel::ColShortOid, 80);
    m_logView->setColumnWidth(GitLogModel::ColAuthor,  130);
    m_logView->setColumnWidth(GitLogModel::ColDate,    130);
    m_logView->verticalHeader()->hide();
    m_logView->setAlternatingRowColors(true);
    m_logView->setShowGrid(false);
    vSplitter->addWidget(m_logView);

    m_statusList = new QListWidget(this);
    m_statusList->setMaximumHeight(150);
    vSplitter->addWidget(m_statusList);

    m_diffView = new GitDiffView(this);
    vSplitter->addWidget(m_diffView);

    vSplitter->setStretchFactor(0, 3);
    vSplitter->setStretchFactor(1, 1);
    vSplitter->setStretchFactor(2, 3);

    hSplitter->addWidget(vSplitter);
    hSplitter->setStretchFactor(0, 0);
    hSplitter->setStretchFactor(1, 1);

    rootLayout->addWidget(hSplitter);

    // ── Status bar ────────────────────────────────────────────────────────────
    m_statusLabel = new QLabel("No repository open.", this);
    rootLayout->addWidget(m_statusLabel);

    // ── Wire toolbar actions ──────────────────────────────────────────────────
    connect(actOpen,   &QAction::triggered, this, &GitClientPanel::onOpenClicked);
    connect(actFetch,  &QAction::triggered, this, &GitClientPanel::onFetchClicked);
    connect(actPull,   &QAction::triggered, this, &GitClientPanel::onPullClicked);
    connect(actPush,   &QAction::triggered, this, &GitClientPanel::onPushClicked);
    connect(actCommit, &QAction::triggered, this, &GitClientPanel::onCommitClicked);
}

void GitClientPanel::setupConnections() {
    // Repo signals
    connect(m_repo, &GitRepository::repositoryOpened, this, [this](const QString &p) {
        m_statusLabel->setText(QString("Repository: %1  [%2]")
                               .arg(p, m_repo->currentBranch()));
        updateBranchCombo();
        refreshStatus();
    });

    connect(m_repo, &GitRepository::branchChanged, this, [this](const QString &b) {
        m_statusLabel->setText(QString("Repository: %1  [%2]")
                               .arg(m_repo->workdir(), b));
        updateBranchCombo();
        refreshStatus();
    });

    connect(m_repo, &GitRepository::statusChanged,   this, &GitClientPanel::refreshStatus);
    connect(m_repo, &GitRepository::operationStarted, this, [this](const QString &desc) {
        m_statusLabel->setText(desc);
        QApplication::setOverrideCursor(Qt::WaitCursor);
    });

    connect(m_repo, &GitRepository::operationFinished, this, &GitClientPanel::onOperationFinished);
    connect(m_repo, &GitRepository::credentialsRequired, this, &GitClientPanel::onCredentialsRequired);

    // Log selection → diff
    connect(m_logView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &GitClientPanel::onLogSelectionChanged);

    // Status list click → diff for that file
    connect(m_statusList, &QListWidget::itemClicked,
            this, &GitClientPanel::onStatusItemClicked);

    // Branch combo switch
    connect(m_branchCombo, &QComboBox::currentTextChanged, this, [this](const QString &name) {
        if (name.isEmpty() || name == m_repo->currentBranch()) return;
        m_repo->checkout(name);
    });
}

// ─── Toolbar slots ────────────────────────────────────────────────────────────

void GitClientPanel::onOpenClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Open Repository", QDir::homePath());
    if (dir.isEmpty()) return;
    openRepository(dir);
}

void GitClientPanel::onFetchClicked() {
    if (!m_repo->isOpen()) return;
    QtConcurrent::run([this]{ m_repo->fetch(); });
}

void GitClientPanel::onPullClicked() {
    if (!m_repo->isOpen()) return;
    QtConcurrent::run([this]{ m_repo->pull(); });
}

void GitClientPanel::onPushClicked() {
    if (!m_repo->isOpen()) return;
    QtConcurrent::run([this]{ m_repo->push(); });
}

void GitClientPanel::onCommitClicked() {
    if (!m_repo->isOpen()) return;
    bool ok = false;
    QString msg = QInputDialog::getMultiLineText(this, "Commit", "Commit message:", {}, &ok);
    if (!ok || msg.trimmed().isEmpty()) return;
    m_repo->commit(msg.trimmed());
}

// ─── Repository event slots ──────────────────────────────────────────────────

void GitClientPanel::onCredentialsRequired(const QString &url, const QString &usernameHint) {
    // This is emitted on the git thread; must marshal to UI thread
    QMetaObject::invokeMethod(this, [this, url, usernameHint] {
        GitCredentialDialog dlg(url, usernameHint, this);
        if (dlg.exec() == QDialog::Accepted) {
            // Store credentials in the repo's Impl via a public setter
            // (we expose a friend method here via a protected signal route)
            // Simplest: re-emit with credentials – handled by a dedicated slot
            emit m_repo->credentialsRequired(url, usernameHint);
            // In a real integration you'd call m_repo->provideCredentials()
        }
    }, Qt::BlockingQueuedConnection);
}

void GitClientPanel::onOperationFinished(bool success, const QString &message) {
    QApplication::restoreOverrideCursor();
    m_statusLabel->setText(message);
    if (!success)
        QMessageBox::warning(this, "Git Error", message);
    refreshStatus();
}

void GitClientPanel::onLogSelectionChanged(const QModelIndex &current, const QModelIndex &) {
    if (!current.isValid()) return;
    // Show cached diff for that commit (simplified: show working-tree diff)
    m_diffView->showDiff(m_repo->diff());
}

void GitClientPanel::onStatusItemClicked(QListWidgetItem *item) {
    const QString path = item->data(Qt::UserRole).toString();
    m_diffView->showDiff(m_repo->diff(path));
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void GitClientPanel::refreshStatus() {
    m_statusList->clear();
    static const QMap<StatusEntry::State, QString> prefix = {
        {StatusEntry::State::Untracked, "? "},
        {StatusEntry::State::Modified,  "M "},
        {StatusEntry::State::Added,     "A "},
        {StatusEntry::State::Deleted,   "D "},
        {StatusEntry::State::Renamed,   "R "},
        {StatusEntry::State::Conflicted,"C "},
        {StatusEntry::State::Ignored,   "! "},
    };
    for (const StatusEntry &se : m_repo->status()) {
        auto *item = new QListWidgetItem(
            prefix.value(se.workdirStatus, "  ") + se.path);
        item->setData(Qt::UserRole, se.path);
        m_statusList->addItem(item);
    }
    m_logModel->refresh(m_repo->currentBranch());
}

void GitClientPanel::updateBranchCombo() {
    QSignalBlocker blocker(m_branchCombo);
    m_branchCombo->clear();
    const QString current = m_repo->currentBranch();
    for (const BranchInfo &b : m_repo->branches(false))
        m_branchCombo->addItem(b.name);
    m_branchCombo->setCurrentText(current);
}

} // namespace Kayte
