#pragma once
// XmlEditorWindow.h – libreparser
// A small standalone editor for "window" XML documents: source on one side,
// parsed tree + validation diagnostics ("debug" output) on the other.

#include <QMainWindow>
#include "UiXmlDocument.h"

class QPlainTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QLabel;

class XmlEditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit XmlEditorWindow(QWidget *parent = nullptr);

    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNew();
    void onOpen();
    bool onSave();
    bool onSaveAs();
    void onParse();
    void onSourceChanged();

private:
    void setupUi();
    void setupActions();
    void loadSource(const QString &xml, const QString &path);
    bool saveToPath(const QString &path);
    void populateTree(const XmlNode &node, QTreeWidgetItem *parentItem);
    void populateDiagnostics(const QList<XmlDiagnostic> &diagnostics);
    void updateWindowTitle();

    QPlainTextEdit *m_sourceEdit { nullptr };
    QTreeWidget    *m_treeView   { nullptr };
    QTableWidget   *m_diagTable  { nullptr };
    QLabel         *m_summary    { nullptr };

    UiXmlDocument   m_document;
    QString         m_currentPath;
    bool            m_dirty { false };
};
