// SvnModel.cpp – KaytEIDE SVN item model
// C++17, Qt 6

#include "SvnModel.h"
#include <QColor>
#include <QFont>
#include <QFileInfo>

namespace Kayte::Svn {

SvnModel::SvnModel(QObject *parent) : QAbstractTableModel(parent) {}

// ── QAbstractTableModel ──────────────────────────────────────────────────────

int SvnModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_filtered.size());
}

int SvnModel::columnCount(const QModelIndex &) const
{
    return static_cast<int>(Column::Count);
}

QVariant SvnModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) return {};

    const StatusEntry &e = m_all[m_filtered[index.row()]];

    switch (role) {
    case Qt::DisplayRole:
        switch (static_cast<Column>(index.column())) {
        case Column::Status:   return statusText(e.textStatus);
        case Column::Filename: return QFileInfo(e.path).fileName();
        case Column::Author:   return e.author;
        case Column::Revision: return e.revision >= 0 ? QString::number(e.revision) : QStringLiteral("-");
        case Column::Date:     return e.lastChanged.isValid()
                                      ? e.lastChanged.toLocalTime().toString(Qt::ISODate) : QString{};
        default: break;
        }
        break;

    case Qt::ToolTipRole:
        return e.path;

    case Qt::ForegroundRole:
        return statusColor(e.textStatus);

    case Qt::FontRole:
        if (e.textStatus == StatusKind::Modified ||
            e.textStatus == StatusKind::Conflicted) {
            QFont f;
            f.setBold(true);
            return f;
        }
        break;

    case Qt::CheckStateRole:
        if (index.column() == static_cast<int>(Column::Filename))
            return m_checked.contains(index.row()) ? Qt::Checked : Qt::Unchecked;
        break;

    case StatusRole:
        return QVariant::fromValue(e.textStatus);

    case FullPathRole:
        return e.path;

    case EntryRole:
        return QVariant::fromValue(e);

    default:
        break;
    }
    return {};
}

QVariant SvnModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (static_cast<Column>(section)) {
    case Column::Status:   return QStringLiteral("Status");
    case Column::Filename: return QStringLiteral("File");
    case Column::Author:   return QStringLiteral("Author");
    case Column::Revision: return QStringLiteral("Rev");
    case Column::Date:     return QStringLiteral("Last Changed");
    default: return {};
    }
}

Qt::ItemFlags SvnModel::flags(const QModelIndex &index) const
{
    auto f = QAbstractTableModel::flags(index);
    if (index.column() == static_cast<int>(Column::Filename))
        f |= Qt::ItemIsUserCheckable;
    return f;
}

bool SvnModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && index.column() == static_cast<int>(Column::Filename)) {
        if (value.toInt() == Qt::Checked)
            m_checked.insert(index.row());
        else
            m_checked.remove(index.row());
        Q_EMIT dataChanged(index, index, { Qt::CheckStateRole });
        return true;
    }
    return false;
}

// ── Data management ──────────────────────────────────────────────────────────

void SvnModel::setEntries(const QList<StatusEntry> &entries)
{
    beginResetModel();
    m_all = entries;
    m_checked.clear();
    rebuildFilter();
    endResetModel();
}

void SvnModel::clearEntries()
{
    setEntries({});
}

const StatusEntry &SvnModel::entryAt(int row) const
{
    return m_all[m_filtered[row]];
}

QList<StatusEntry> SvnModel::checkedEntries() const
{
    QList<StatusEntry> result;
    for (int row : m_checked) {
        if (row < static_cast<int>(m_filtered.size()))
            result.append(m_all[m_filtered[row]]);
    }
    return result;
}

// ── Filtering ────────────────────────────────────────────────────────────────

void SvnModel::setShowUnversioned(bool show)
{
    if (m_showUnversioned == show) return;
    m_showUnversioned = show;
    beginResetModel();
    rebuildFilter();
    endResetModel();
}

void SvnModel::setShowNormal(bool show)
{
    if (m_showNormal == show) return;
    m_showNormal = show;
    beginResetModel();
    rebuildFilter();
    endResetModel();
}

void SvnModel::setAllChecked(bool checked)
{
    m_checked.clear();
    if (checked) {
        for (int i = 0; i < static_cast<int>(m_filtered.size()); ++i)
            m_checked.insert(i);
    }
    if (!m_filtered.isEmpty()) {
        Q_EMIT dataChanged(index(0, static_cast<int>(Column::Filename)),
                           index(rowCount()-1, static_cast<int>(Column::Filename)),
                           { Qt::CheckStateRole });
    }
}

void SvnModel::rebuildFilter()
{
    m_filtered.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        if (passesFilter(m_all[i]))
            m_filtered.append(i);
    }
}

bool SvnModel::passesFilter(const StatusEntry &e) const
{
    if (!m_showUnversioned && e.textStatus == StatusKind::Unversioned) return false;
    if (!m_showNormal      && e.textStatus == StatusKind::Normal)      return false;
    return true;
}

// ── Helpers ──────────────────────────────────────────────────────────────────

QString SvnModel::statusText(StatusKind k)
{
    switch (k) {
    case StatusKind::None:        return {};
    case StatusKind::Unversioned: return QStringLiteral("?");
    case StatusKind::Normal:      return QStringLiteral("·");
    case StatusKind::Added:       return QStringLiteral("A");
    case StatusKind::Missing:     return QStringLiteral("!");
    case StatusKind::Deleted:     return QStringLiteral("D");
    case StatusKind::Replaced:    return QStringLiteral("R");
    case StatusKind::Modified:    return QStringLiteral("M");
    case StatusKind::Merged:      return QStringLiteral("G");
    case StatusKind::Conflicted:  return QStringLiteral("C");
    case StatusKind::Ignored:     return QStringLiteral("I");
    case StatusKind::Obstructed:  return QStringLiteral("~");
    case StatusKind::External:    return QStringLiteral("X");
    case StatusKind::Incomplete:  return QStringLiteral("%");
    }
    return {};
}

QColor SvnModel::statusColor(StatusKind k)
{
    switch (k) {
    case StatusKind::Added:       return QColor(0x2e7d32); // dark green
    case StatusKind::Deleted:     return QColor(0xc62828); // dark red
    case StatusKind::Modified:    return QColor(0x1565c0); // dark blue
    case StatusKind::Conflicted:  return QColor(0xe65100); // orange
    case StatusKind::Missing:     return QColor(0xad1457); // pink
    case StatusKind::Unversioned: return QColor(0x757575); // grey
    case StatusKind::Replaced:    return QColor(0x6a1b9a); // purple
    default:                      return QColor(Qt::black);
    }
}

} // namespace Kayte::Svn
