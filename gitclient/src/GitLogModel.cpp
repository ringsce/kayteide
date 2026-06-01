#include "GitLogModel.hpp"
#include <QFont>
#include <QBrush>
#include <QColor>

namespace Kayte {

GitLogModel::GitLogModel(GitRepository *repo, QObject *parent)
    : QAbstractTableModel(parent), m_repo(repo)
{
    connect(repo, &GitRepository::branchChanged, this, [this]{ refresh(); });
    connect(repo, &GitRepository::repositoryOpened, this, [this]{ refresh(); });
}

void GitLogModel::refresh(const QString &branch, int maxCount) {
    beginResetModel();
    m_log = m_repo->log(branch, maxCount);
    endResetModel();
}

int GitLogModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_log.size();
}

int GitLogModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : ColCount;
}

const CommitInfo &GitLogModel::commitAt(int row) const {
    return m_log.at(row);
}

QVariant GitLogModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case ColGraph:    return "Graph";
    case ColShortOid: return "SHA";
    case ColSummary:  return "Summary";
    case ColAuthor:   return "Author";
    case ColDate:     return "Date";
    }
    return {};
}

QVariant GitLogModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_log.size()) return {};
    const CommitInfo &ci = m_log.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColGraph:    return {};   // painted by delegate
        case ColShortOid: return ci.shortOid;
        case ColSummary:  return ci.summary;
        case ColAuthor:   return ci.authorName;
        case ColDate:     return ci.timestamp.toLocalTime().toString("yyyy-MM-dd HH:mm");
        }
    }

    if (role == Qt::FontRole && index.column() == ColShortOid) {
        QFont f;
        f.setFamily("monospace");
        f.setPointSize(9);
        return f;
    }

    if (role == Qt::ForegroundRole && index.column() == ColShortOid)
        return QBrush(QColor(0x00, 0x87, 0xFF));

    if (role == Qt::UserRole)
        return ci.oid;   // full OID accessible via UserRole

    return {};
}

} // namespace Kayte
