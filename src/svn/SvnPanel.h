#pragma once
// SvnPanel.h – QDockWidget that hosts the full SVN client UI within KaytEIDE.
// Embed into the IDE via:
//   auto *panel = new Kayte::Svn::SvnPanel(this);
//   addDockWidget(Qt::BottomDockWidgetArea, panel);
// C++17, Qt 6

#include "SvnTypes.h"
#include <QDockWidget>
#include <memory>

QT_BEGIN_NAMESPACE
class QTableView;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QToolBar;
class QSplitter;
class QLabel;
class QMenu;
class QComboBox;
QT_END_NAMESPACE

namespace Kayte::Svn {

class SvnClient;
class SvnModel;

class SvnPanel : public QDockWidget
{
    Q_OBJECT

public:
    explicit SvnPanel(QWidget *parent = nullptr);
    ~SvnPanel() override;

    // Call this to point the panel at a working-copy directory.
    void setWorkingCopy(const QString &wcPath);
    [[nodiscard]] QString workingCopy() const;

    // Access the underlying client (connect extra signals if needed).
    [[nodiscard]] SvnClient *client() const;

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void openFileRequested(const QString &filePath);
    void openDiffRequested(const QString &filePath, qint64 rev1, qint64 rev2);

private Q_SLOTS:
    // Internal toolbar actions
    void onRefresh();
    void onUpdate();
    void onCommit();
    void onRevert();
    void onAdd();
    void onDelete();
    void onShowLog();
    void onShowDiff();
    void onCleanup();

    // SvnClient signals
    void onStatusReady(const QList<Kayte::Svn::StatusEntry> &entries);
    void onError(const QString &op, const QString &msg);
    void onOperationDone(const QString &op, bool ok, const QString &output);

    // Table context menu
    void onContextMenu(const QPoint &pos);

    // Double-click opens file
    void onActivated(const QModelIndex &index);

private:
    void setupUi();
    void setupToolbar();
    void setupConnections();
    void setStatus(const QString &msg);
    void appendOutput(const QString &text);
    QStringList selectedPaths() const;

    struct Ui {
        QToolBar   *toolbar   { nullptr };
        QSplitter  *splitter  { nullptr };
        QTableView *table     { nullptr };
        QPlainTextEdit *output { nullptr };
        QLineEdit  *wcEdit    { nullptr };
        QLabel     *statusBar { nullptr };
        QPushButton *browseBtn{ nullptr };
    } m_ui;

    SvnClient *m_client { nullptr };
    SvnModel  *m_model  { nullptr };
    QString    m_wcPath;
};

} // namespace Kayte::Svn
