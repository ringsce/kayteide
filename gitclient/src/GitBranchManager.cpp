#include "GitBranchManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>

namespace Kayte {

GitBranchManager::GitBranchManager(GitRepository *repo, QWidget *parent)
    : QWidget(parent), m_repo(repo)
{
    buildUi();
    setupConnections();
    refresh();
}

void GitBranchManager::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4,4,4,4);
    mainLayout->setSpacing(4);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({"Branch", "↑", "↓"});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->setRootIsDecorated(true);
    mainLayout->addWidget(m_tree);

    auto *btnRow = new QHBoxLayout;
    m_btnCheckout = new QPushButton("Checkout", this);
    m_btnNew      = new QPushButton("New…",     this);
    m_btnDelete   = new QPushButton("Delete",   this);
    btnRow->addWidget(m_btnCheckout);
    btnRow->addWidget(m_btnNew);
    btnRow->addWidget(m_btnDelete);
    mainLayout->addLayout(btnRow);
}

void GitBranchManager::setupConnections() {
    connect(m_btnCheckout, &QPushButton::clicked, this, [this] {
        auto *item = m_tree->currentItem();
        if (!item || item->parent() == nullptr) return; // top-level category
        emit branchCheckoutRequested(item->text(0));
    });

    connect(m_btnNew, &QPushButton::clicked, this, [this] {
        bool ok = false;
        QString name = QInputDialog::getText(this, "New Branch", "Branch name:", QLineEdit::Normal, {}, &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        if (m_repo->createBranch(name.trimmed()))
            refresh();
        else
            QMessageBox::warning(this, "Error", m_repo->lastError());
    });

    connect(m_btnDelete, &QPushButton::clicked, this, [this] {
        auto *item = m_tree->currentItem();
        if (!item || item->parent() == nullptr) return;
        const QString name = item->text(0);
        auto res = QMessageBox::question(this, "Delete Branch",
                                         QString("Delete branch '%1'?").arg(name));
        if (res != QMessageBox::Yes) return;
        if (m_repo->deleteBranch(name))
            refresh();
        else
            QMessageBox::warning(this, "Error", m_repo->lastError());
    });

    connect(m_repo, &GitRepository::branchChanged, this, &GitBranchManager::refresh);
    connect(m_repo, &GitRepository::repositoryOpened, this, &GitBranchManager::refresh);
}

void GitBranchManager::refresh() {
    m_tree->clear();
    auto *localCat  = new QTreeWidgetItem(m_tree, QStringList{"Local"});
    auto *remoteCat = new QTreeWidgetItem(m_tree, QStringList{"Remote"});
    localCat->setFlags(localCat->flags() & ~Qt::ItemIsSelectable);
    remoteCat->setFlags(remoteCat->flags() & ~Qt::ItemIsSelectable);

    const QString head = m_repo->currentBranch();
    for (const BranchInfo &b : m_repo->branches(true)) {
        auto *item = new QTreeWidgetItem;
        item->setText(0, b.name);
        item->setText(1, b.aheadCount  > 0 ? QString::number(b.aheadCount)  : "");
        item->setText(2, b.behindCount > 0 ? QString::number(b.behindCount) : "");

        QFont f = item->font(0);
        if (b.isHead) { f.setBold(true); item->setFont(0, f); }
        (b.isLocal ? localCat : remoteCat)->addChild(item);
    }

    localCat->setExpanded(true);
    remoteCat->setExpanded(true);
}

} // namespace Kayte
