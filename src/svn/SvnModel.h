#pragma once
// SvnModel.h – QAbstractTableModel backed by a list of StatusEntry objects.
// C++17, Qt 6

#include "SvnTypes.h"   // StatusEntry, StatusKind – Q_DECLARE_METATYPE is in SvnTypes.h

#include <QAbstractTableModel>
#include <QList>
#include <QSet>
#include <QColor>

namespace Kayte::Svn {

class SvnModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum class Column : int {
        Status   = 0,
        Filename = 1,
        Author   = 2,
        Revision = 3,
        Date     = 4,
        Count    = 5
    };

    // Custom roles
    enum Roles {
        StatusRole   = Qt::UserRole + 1,  // StatusKind
        FullPathRole = Qt::UserRole + 2,
        EntryRole    = Qt::UserRole + 3,  // StatusEntry as QVariant
    };

    explicit SvnModel(QObject *parent = nullptr);

    // ── QAbstractTableModel interface ────────────────────────────────────────
    [[nodiscard]] int rowCount   (const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data      (const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    // ── Data management ──────────────────────────────────────────────────────
    void setEntries(const QList<StatusEntry> &entries);
    void clearEntries();
    [[nodiscard]] const StatusEntry &entryAt(int row) const;
    [[nodiscard]] QList<StatusEntry> checkedEntries() const;

    // ── Filtering ────────────────────────────────────────────────────────────
    void setShowUnversioned(bool show);
    void setShowNormal(bool show);
    [[nodiscard]] bool showUnversioned() const { return m_showUnversioned; }
    [[nodiscard]] bool showNormal()      const { return m_showNormal; }

    // ── Checkboxes ───────────────────────────────────────────────────────────
    void setAllChecked(bool checked);

private:
    [[nodiscard]] bool passesFilter(const StatusEntry &e) const;
    static QString statusText (StatusKind k);
    static QColor  statusColor(StatusKind k);
    void rebuildFilter();

    QList<StatusEntry> m_all;
    QList<int>         m_filtered;
    QSet<int>          m_checked;

    bool m_showUnversioned { true  };
    bool m_showNormal      { false };
};

} // namespace Kayte::Svn

// NOTE: Q_DECLARE_METATYPE for Kayte::Svn types lives in SvnTypes.h.
//       Do NOT add Q_DECLARE_METATYPE calls here – duplicate declarations
//       cause "explicit specialization after instantiation" errors in the
//       moc-generated unity build (mocs_compilation.cpp).
