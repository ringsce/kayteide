#pragma once
#include <QAbstractTableModel>
#include "GitRepository.hpp"

namespace Kayte {

class GitLogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColGraph=0, ColShortOid, ColSummary, ColAuthor, ColDate, ColCount };

    explicit GitLogModel(GitRepository *repo, QObject *parent = nullptr);

    void refresh(const QString &branch = {}, int maxCount = 500);

    // QAbstractTableModel
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    const CommitInfo &commitAt(int row) const;

private:
    GitRepository       *m_repo {nullptr};
    QVector<CommitInfo>  m_log;
};

} // namespace Kayte
