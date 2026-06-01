#pragma once
// SvnLogDialog.h – Modeless dialog showing SVN log for a path
// C++17, Qt 6

#include "SvnTypes.h"

// Full Qt includes required here so that moc can resolve base-class names
// when it processes this header independently of the .cpp translation unit.
#include <QAbstractTableModel>
#include <QDialog>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QSplitter;
class QLabel;
QT_END_NAMESPACE

namespace Kayte::Svn {

class SvnClient;

// ── Log table model ──────────────────────────────────────────────────────────
// Declared in this header (not a separate file) to keep the log dialog
// self-contained.  moc requires that all base classes are fully defined in
// scope when it parses the Q_OBJECT class, so QAbstractTableModel must be
// #included (not just forward-declared) above.
class SvnLogModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SvnLogModel(QObject *parent = nullptr);
    void setEntries(const QList<LogEntry> &entries);

    int      rowCount   (const QModelIndex &parent = QModelIndex()) const override;
    int      columnCount(const QModelIndex &parent = QModelIndex()) const override { return 4; }
    QVariant data       (const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData (int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole) const override;

    [[nodiscard]] const LogEntry &entryAt(int row) const;

private:
    QList<LogEntry> m_entries;
};

// ── Log dialog ───────────────────────────────────────────────────────────────
class SvnLogDialog : public QDialog
{
    Q_OBJECT
public:
    SvnLogDialog(SvnClient *client, const QString &path, QWidget *parent = nullptr);

private Q_SLOTS:
    void onLogReady(const QList<Kayte::Svn::LogEntry> &entries);
    void onSelectionChanged();

private:
    SvnClient   *m_client  { nullptr };
    SvnLogModel *m_model   { nullptr };
    QTableView  *m_table   { nullptr };
    QTextEdit   *m_msgView { nullptr };
    QTextEdit   *m_paths   { nullptr };
    QString      m_path;
};

} // namespace Kayte::Svn
