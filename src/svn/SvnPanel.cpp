// SvnPanel.cpp – KaytEIDE SVN dock panel
// C++17, Qt 6

#include "SvnPanel.h"
#include "SvnClient.h"
#include "SvnModel.h"
#include "SvnLogDialog.h"
#include "SvnCommitDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>
#include <QSplitter>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QHeaderView>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QApplication>
#include <QDateTime>
#include <QSortFilterProxyModel>

namespace Kayte::Svn {

// ────────────────────────────────────────────────────────────────────────────
SvnPanel::SvnPanel(QWidget *parent)
    : QDockWidget(tr("Subversion"), parent)
    , m_client(new SvnClient(this))
    , m_model(new SvnModel(this))
{
    setObjectName(QStringLiteral("SvnPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setupUi();
    setupConnections();
}

SvnPanel::~SvnPanel() = default;

// ── Public API ────────────────────────────────────────────────────────────

void SvnPanel::setWorkingCopy(const QString &wcPath)
{
    m_wcPath = wcPath;
    m_ui.wcEdit->setText(wcPath);
    refresh();
}

QString SvnPanel::workingCopy() const { return m_wcPath; }
SvnClient *SvnPanel::client() const  { return m_client; }

void SvnPanel::refresh() { onRefresh(); }

// ── UI setup ──────────────────────────────────────────────────────────────

void SvnPanel::setupUi()
{
    auto *root    = new QWidget(this);
    auto *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(2);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    m_ui.toolbar = new QToolBar(this);
    m_ui.toolbar->setIconSize({ 16, 16 });
    setupToolbar();
    rootLay->addWidget(m_ui.toolbar);

    // ── WC path row ──────────────────────────────────────────────────────────
    auto *pathRow = new QHBoxLayout;
    m_ui.wcEdit   = new QLineEdit(this);
    m_ui.wcEdit->setPlaceholderText(tr("Working copy path…"));
    m_ui.browseBtn = new QPushButton(tr("…"), this);
    m_ui.browseBtn->setFixedWidth(30);
    pathRow->addWidget(new QLabel(tr("WC:"), this));
    pathRow->addWidget(m_ui.wcEdit);
    pathRow->addWidget(m_ui.browseBtn);
    rootLay->addLayout(pathRow);

    // ── Splitter (table | output) ────────────────────────────────────────────
    m_ui.splitter = new QSplitter(Qt::Vertical, this);

    // Status table
    m_ui.table = new QTableView(this);
    auto *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(m_model);
    proxy->setSortRole(Qt::DisplayRole);
    m_ui.table->setModel(proxy);
    m_ui.table->setSortingEnabled(true);
    m_ui.table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui.table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_ui.table->setAlternatingRowColors(true);
    m_ui.table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui.table->horizontalHeader()->setStretchLastSection(false);
    m_ui.table->horizontalHeader()->setSectionResizeMode(
        static_cast<int>(SvnModel::Column::Filename), QHeaderView::Stretch);
    m_ui.table->setColumnWidth(static_cast<int>(SvnModel::Column::Status),   30);
    m_ui.table->setColumnWidth(static_cast<int>(SvnModel::Column::Revision), 60);
    m_ui.table->setColumnWidth(static_cast<int>(SvnModel::Column::Author),   90);
    m_ui.table->setColumnWidth(static_cast<int>(SvnModel::Column::Date),    140);
    m_ui.table->verticalHeader()->hide();

    // Output area
    m_ui.output = new QPlainTextEdit(this);
    m_ui.output->setReadOnly(true);
    m_ui.output->setMaximumBlockCount(1000);
    m_ui.output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_ui.output->setMinimumHeight(80);

    m_ui.splitter->addWidget(m_ui.table);
    m_ui.splitter->addWidget(m_ui.output);
    m_ui.splitter->setStretchFactor(0, 3);
    m_ui.splitter->setStretchFactor(1, 1);
    rootLay->addWidget(m_ui.splitter, 1);

    // ── Status bar ───────────────────────────────────────────────────────────
    m_ui.statusBar = new QLabel(tr("Ready"), this);
    m_ui.statusBar->setIndent(4);
    rootLay->addWidget(m_ui.statusBar);

    setWidget(root);
}

void SvnPanel::setupToolbar()
{
    auto *tb = m_ui.toolbar;

    auto *refreshAct = tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_BrowserReload),
        tr("Refresh"), this, &SvnPanel::onRefresh);
    refreshAct->setShortcut(QKeySequence::Refresh);

    tb->addSeparator();

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_ArrowDown),
        tr("Update"), this, &SvnPanel::onUpdate);

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_ArrowUp),
        tr("Commit…"), this, &SvnPanel::onCommit);

    tb->addSeparator();

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_FileIcon),
        tr("Add"), this, &SvnPanel::onAdd);

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_TrashIcon),
        tr("Delete"), this, &SvnPanel::onDelete);

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_DialogResetButton),
        tr("Revert"), this, &SvnPanel::onRevert);

    tb->addSeparator();

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView),
        tr("Log…"), this, &SvnPanel::onShowLog);

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView),
        tr("Diff"), this, &SvnPanel::onShowDiff);

    tb->addSeparator();

    tb->addAction(
        QApplication::style()->standardIcon(QStyle::SP_BrowserStop),
        tr("Cleanup"), this, &SvnPanel::onCleanup);
}

void SvnPanel::setupConnections()
{
    // Browse button
    connect(m_ui.browseBtn, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select Working Copy"), m_wcPath);
        if (!dir.isEmpty()) setWorkingCopy(dir);
    });

    // WC edit – refresh on enter
    connect(m_ui.wcEdit, &QLineEdit::returnPressed, this, [this] {
        setWorkingCopy(m_ui.wcEdit->text().trimmed());
    });

    // Table context menu
    connect(m_ui.table, &QWidget::customContextMenuRequested,
            this, &SvnPanel::onContextMenu);
    connect(m_ui.table, &QAbstractItemView::activated,
            this, &SvnPanel::onActivated);

    // SVN client signals
    connect(m_client, &SvnClient::statusReady,
            this, &SvnPanel::onStatusReady);
    connect(m_client, &SvnClient::errorOccurred,
            this, &SvnPanel::onError);
    connect(m_client, &SvnClient::operationFinished,
            this, &SvnPanel::onOperationDone);
    connect(m_client, &SvnClient::updateFinished,
            this, [this](const QStringList &, qint64 rev) {
        appendOutput(tr("Updated to revision %1.").arg(rev));
        refresh();
    });
    connect(m_client, &SvnClient::commitFinished,
            this, [this](const CommitResult &r) {
        if (r.success)
            appendOutput(tr("Committed revision %1.").arg(r.newRevision));
        else
            appendOutput(tr("Commit failed: %1").arg(r.error));
        refresh();
    });
}

// ── Toolbar slots ─────────────────────────────────────────────────────────

void SvnPanel::onRefresh()
{
    if (m_wcPath.isEmpty()) return;
    setStatus(tr("Refreshing…"));
    m_client->status(m_wcPath, true);
}

void SvnPanel::onUpdate()
{
    if (m_wcPath.isEmpty()) return;
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) paths = { m_wcPath };
    setStatus(tr("Updating…"));
    m_client->update(paths);
}

void SvnPanel::onCommit()
{
    const QList<StatusEntry> checked = m_model->checkedEntries();
    QStringList paths;
    for (const auto &e : checked) paths << e.path;
    if (paths.isEmpty()) paths = selectedPaths();
    if (paths.isEmpty()) { QMessageBox::information(this, tr("Commit"), tr("No files selected.")); return; }

    SvnCommitDialog dlg(paths, this);
    if (dlg.exec() != QDialog::Accepted) return;

    setStatus(tr("Committing…"));
    m_client->commit(paths, dlg.message());
}

void SvnPanel::onRevert()
{
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) return;
    if (QMessageBox::question(this, tr("Revert"),
            tr("Revert %1 file(s)? All local changes will be lost.").arg(paths.size()))
        != QMessageBox::Yes) return;
    m_client->revert(paths);
    setStatus(tr("Reverting…"));
}

void SvnPanel::onAdd()
{
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) return;
    m_client->add(paths);
    setStatus(tr("Adding…"));
}

void SvnPanel::onDelete()
{
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) return;
    if (QMessageBox::question(this, tr("Delete"),
            tr("Schedule %1 file(s) for deletion?").arg(paths.size()))
        != QMessageBox::Yes) return;
    m_client->remove(paths);
    setStatus(tr("Deleting…"));
}

void SvnPanel::onShowLog()
{
    const auto paths = selectedPaths();
    const QString path = paths.isEmpty() ? m_wcPath : paths.first();
    if (path.isEmpty()) return;
    auto *dlg = new SvnLogDialog(m_client, path, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void SvnPanel::onShowDiff()
{
    const auto paths = selectedPaths();
    if (paths.isEmpty()) return;
    Q_EMIT openDiffRequested(paths.first(), -1, -1);
    m_client->diff(paths.first());
    connect(m_client, &SvnClient::diffReady, this, [this](const DiffResult &dr) {
        appendOutput(dr.unifiedDiff.left(4000)); // preview first 4k chars
    }, Qt::SingleShotConnection);
}

void SvnPanel::onCleanup()
{
    if (m_wcPath.isEmpty()) return;
    m_client->cleanup(m_wcPath);
    setStatus(tr("Cleaning up…"));
}

// ── Client signal handlers ────────────────────────────────────────────────

void SvnPanel::onStatusReady(const QList<StatusEntry> &entries)
{
    m_model->setEntries(entries);
    setStatus(tr("%1 file(s) — %2")
        .arg(entries.size())
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
}

void SvnPanel::onError(const QString &op, const QString &msg)
{
    appendOutput(tr("[ERROR] %1: %2").arg(op, msg));
    setStatus(tr("Error in %1").arg(op));
}

void SvnPanel::onOperationDone(const QString &op, bool ok, const QString &output)
{
    appendOutput(tr("[%1] %2\n%3").arg(ok ? tr("OK") : tr("FAIL"), op, output));
    if (ok) refresh();
}

// ── Context menu ──────────────────────────────────────────────────────────

void SvnPanel::onContextMenu(const QPoint &pos)
{
    const QModelIndex idx = m_ui.table->indexAt(pos);
    if (!idx.isValid()) return;

    QMenu menu(this);
    menu.addAction(tr("Open"),        this, [this, idx]{ onActivated(idx); });
    menu.addAction(tr("Diff"),        this, &SvnPanel::onShowDiff);
    menu.addAction(tr("Log…"),        this, &SvnPanel::onShowLog);
    menu.addSeparator();
    menu.addAction(tr("Add"),         this, &SvnPanel::onAdd);
    menu.addAction(tr("Revert"),      this, &SvnPanel::onRevert);
    menu.addAction(tr("Delete"),      this, &SvnPanel::onDelete);
    menu.addSeparator();
    menu.addAction(tr("Resolve (mine)"), this, [this]{
        m_client->resolve(selectedPaths(), SvnClient::ResolveChoice::MineFull);
    });
    menu.addAction(tr("Resolve (theirs)"), this, [this]{
        m_client->resolve(selectedPaths(), SvnClient::ResolveChoice::TheirsFull);
    });
    menu.exec(m_ui.table->viewport()->mapToGlobal(pos));
}

void SvnPanel::onActivated(const QModelIndex &index)
{
    const auto *proxy = qobject_cast<QSortFilterProxyModel *>(m_ui.table->model());
    const auto src    = proxy ? proxy->mapToSource(index) : index;
    if (!src.isValid()) return;
    const QString path = m_model->data(m_model->index(src.row(), 0),
                                        SvnModel::FullPathRole).toString();
    if (!path.isEmpty())
        Q_EMIT openFileRequested(path);
}

// ── Helpers ───────────────────────────────────────────────────────────────

void SvnPanel::setStatus(const QString &msg) { m_ui.statusBar->setText(msg); }

void SvnPanel::appendOutput(const QString &text)
{
    m_ui.output->appendPlainText(text);
}

QStringList SvnPanel::selectedPaths() const
{
    const auto *proxy = qobject_cast<QSortFilterProxyModel *>(m_ui.table->model());
    QStringList paths;
    for (const auto &idx : m_ui.table->selectionModel()->selectedRows()) {
        const auto src = proxy ? proxy->mapToSource(idx) : idx;
        const QString p = m_model->data(m_model->index(src.row(), 0),
                                         SvnModel::FullPathRole).toString();
        if (!p.isEmpty()) paths << p;
    }
    return paths;
}

} // namespace Kayte::Svn
