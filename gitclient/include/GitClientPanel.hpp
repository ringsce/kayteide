#pragma once
#include <QWidget>
#include <QSplitter>
#include <QTableView>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include <QToolBar>
#include <QStatusBar>
#include "GitRepository.hpp"
#include "GitLogModel.hpp"
#include "GitDiffView.hpp"
#include "GitBranchManager.hpp"

namespace Kayte {

/**
 * @brief GitClientPanel
 *
 * A self-contained Qt widget that embeds a full Git workflow UI into KayteIDE.
 *
 * Layout (simplified):
 *  ┌─────────────────────────────────────────────────────────────┐
 *  │ Toolbar: [Open] [Fetch] [Pull] [Push] [Commit] branch combo  │
 *  ├──────────────────┬──────────────────────────────────────────┤
 *  │ Branch Manager   │  Commit log (QTableView)                  │
 *  │                  ├──────────────────────────────────────────┤
 *  │                  │  File status (QListWidget)                │
 *  │                  ├──────────────────────────────────────────┤
 *  │                  │  Diff viewer                              │
 *  └──────────────────┴──────────────────────────────────────────┘
 */
class GitClientPanel : public QWidget {
    Q_OBJECT
public:
    explicit GitClientPanel(QWidget *parent = nullptr);

    /// Open or switch to a repository at @p path.
    bool openRepository(const QString &path);

    GitRepository *repository() const;

signals:
    void statusMessage(const QString &msg);

private slots:
    void onOpenClicked();
    void onFetchClicked();
    void onPullClicked();
    void onPushClicked();
    void onCommitClicked();
    void onLogSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void onStatusItemClicked(QListWidgetItem *item);
    void onCredentialsRequired(const QString &url, const QString &usernameHint);
    void onOperationFinished(bool success, const QString &message);
    void refreshStatus();

private:
    void buildUi();
    void setupConnections();
    void updateBranchCombo();

    GitRepository     *m_repo          {nullptr};
    GitLogModel       *m_logModel      {nullptr};

    // UI elements
    QToolBar          *m_toolbar       {nullptr};
    QComboBox         *m_branchCombo   {nullptr};
    GitBranchManager  *m_branchMgr     {nullptr};
    QTableView        *m_logView       {nullptr};
    QListWidget       *m_statusList    {nullptr};
    GitDiffView       *m_diffView      {nullptr};
    QLabel            *m_statusLabel   {nullptr};
};

} // namespace Kayte
