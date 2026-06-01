#pragma once
#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include "GitRepository.hpp"

namespace Kayte {

/// Dockable branch manager panel.
class GitBranchManager : public QWidget {
    Q_OBJECT
public:
    explicit GitBranchManager(GitRepository *repo, QWidget *parent = nullptr);

public slots:
    void refresh();

signals:
    void branchCheckoutRequested(const QString &branch);

private:
    void buildUi();
    void setupConnections();

    GitRepository  *m_repo     {nullptr};
    QTreeWidget    *m_tree     {nullptr};
    QPushButton    *m_btnNew   {nullptr};
    QPushButton    *m_btnDelete{nullptr};
    QPushButton    *m_btnCheckout{nullptr};
};

} // namespace Kayte
