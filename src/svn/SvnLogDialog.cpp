// SvnLogDialog.cpp
// C++17, Qt 6

#include "SvnLogDialog.h"
#include "SvnClient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QTextEdit>
#include <QSplitter>
#include <QLabel>
#include <QHeaderView>
#include <QColor>
#include <QFontDatabase>

namespace Kayte::Svn {

// ── SvnLogModel ─────────────────────────────────────────────────────────────

SvnLogModel::SvnLogModel(QObject *parent) : QAbstractTableModel(parent) {}

void SvnLogModel::setEntries(const QList<LogEntry> &entries)
{
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

int SvnLogModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(m_entries.size());
}

QVariant SvnLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) return {};
    const LogEntry &e = m_entries[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return e.revision;
        case 1: return e.author;
        case 2: return e.date.toLocalTime().toString(Qt::ISODate);
        case 3: return e.message.section(QLatin1Char('\n'), 0, 0); // first line
        }
    }
    return {};
}

QVariant SvnLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case 0: return QStringLiteral("Rev");
    case 1: return QStringLiteral("Author");
    case 2: return QStringLiteral("Date");
    case 3: return QStringLiteral("Message");
    }
    return {};
}

const LogEntry &SvnLogModel::entryAt(int row) const { return m_entries[row]; }

// ── SvnLogDialog ─────────────────────────────────────────────────────────────

SvnLogDialog::SvnLogDialog(SvnClient *client, const QString &path, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_model(new SvnLogModel(this))
    , m_path(path)
{
    setWindowTitle(tr("SVN Log — %1").arg(path));
    setMinimumSize(700, 500);
    setAttribute(Qt::WA_DeleteOnClose);

    auto *lay     = new QVBoxLayout(this);
    auto *splitter= new QSplitter(Qt::Vertical, this);

    // Table
    m_table = new QTableView(this);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setColumnWidth(0, 60);
    m_table->setColumnWidth(1, 90);
    m_table->setColumnWidth(2, 150);

    // Lower pane: message + changed paths
    auto *lowerWidget = new QWidget(this);
    auto *lowerLay    = new QHBoxLayout(lowerWidget);
    lowerLay->setContentsMargins(0, 0, 0, 0);

    m_msgView = new QTextEdit(this);
    m_msgView->setReadOnly(true);
    m_msgView->setPlaceholderText(tr("Commit message…"));

    m_paths = new QTextEdit(this);
    m_paths->setReadOnly(true);
    m_paths->setPlaceholderText(tr("Changed paths…"));
    m_paths->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    lowerLay->addWidget(m_msgView, 2);
    lowerLay->addWidget(m_paths, 1);

    splitter->addWidget(m_table);
    splitter->addWidget(lowerWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    lay->addWidget(splitter);

    // Connections
    connect(m_client, &SvnClient::logReady, this, &SvnLogDialog::onLogReady);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &SvnLogDialog::onSelectionChanged);

    // Fetch
    m_client->log(m_path, 200);
}

void SvnLogDialog::onLogReady(const QList<LogEntry> &entries)
{
    m_model->setEntries(entries);
    if (!entries.isEmpty())
        m_table->selectRow(0);
}

void SvnLogDialog::onSelectionChanged()
{
    const int row = m_table->currentIndex().row();
    if (row < 0 || row >= m_model->rowCount()) return;
    const LogEntry &e = m_model->entryAt(row);
    m_msgView->setPlainText(e.message);
    m_paths->setPlainText(e.changedPaths.join(QLatin1Char('\n')));
}

} // namespace Kayte::Svn
