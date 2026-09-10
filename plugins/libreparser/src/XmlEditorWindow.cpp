#include "XmlEditorWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QColor>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextStream>
#include <QToolBar>
#include <QTreeWidget>

#include <algorithm>

namespace {
constexpr char kDefaultTemplate[] = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE window SYSTEM "ui.dtd">
<window id="main_win">
    <title>New Window</title>
    <content>
        <text>Enter your message here.</text>
    </content>
    <actions>
        <button id="btn_ok" onclick="onOk()">OK</button>
    </actions>
</window>
)XML";
}

XmlEditorWindow::XmlEditorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupActions();
    loadSource(QString::fromLatin1(kDefaultTemplate), QString());
    onParse();
}

void XmlEditorWindow::setupUi()
{
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    m_sourceEdit = new QPlainTextEdit(splitter);
    m_sourceEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont mono(QStringLiteral("Menlo"));
    mono.setStyleHint(QFont::Monospace);
    m_sourceEdit->setFont(mono);
    connect(m_sourceEdit, &QPlainTextEdit::textChanged, this, &XmlEditorWindow::onSourceChanged);

    m_treeView = new QTreeWidget(splitter);
    m_treeView->setHeaderLabels({tr("Element"), tr("Attributes"), tr("Text")});
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    splitter->addWidget(m_sourceEdit);
    splitter->addWidget(m_treeView);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    m_diagTable = new QTableWidget(0, 3, this);
    m_diagTable->setHorizontalHeaderLabels({tr("Severity"), tr("Location"), tr("Message")});
    m_diagTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_diagTable->verticalHeader()->setVisible(false);
    m_diagTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_diagTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *dock = new QDockWidget(tr("Debug Output"), this);
    dock->setWidget(m_diagTable);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    m_summary = new QLabel(this);
    statusBar()->addWidget(m_summary);

    resize(1100, 720);
}

void XmlEditorWindow::setupActions()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    auto *toolbar = addToolBar(tr("Main"));

    auto *newAction = new QAction(tr("&New"), this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &XmlEditorWindow::onNew);

    auto *openAction = new QAction(tr("&Open..."), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &XmlEditorWindow::onOpen);

    auto *saveAction = new QAction(tr("&Save"), this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &XmlEditorWindow::onSave);

    auto *saveAsAction = new QAction(tr("Save &As..."), this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &XmlEditorWindow::onSaveAs);

    auto *parseAction = new QAction(tr("&Parse / Validate"), this);
    parseAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(parseAction, &QAction::triggered, this, &XmlEditorWindow::onParse);

    for (auto *a : {newAction, openAction, saveAction, saveAsAction}) {
        fileMenu->addAction(a);
        toolbar->addAction(a);
    }
    toolsMenu->addAction(parseAction);
    toolbar->addSeparator();
    toolbar->addAction(parseAction);
}

void XmlEditorWindow::openFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Open"), tr("Cannot open %1:\n%2").arg(path, file.errorString()));
        return;
    }
    const QString xml = QString::fromUtf8(file.readAll());
    loadSource(xml, path);
    onParse();
}

void XmlEditorWindow::loadSource(const QString &xml, const QString &path)
{
    m_sourceEdit->blockSignals(true);
    m_sourceEdit->setPlainText(xml);
    m_sourceEdit->blockSignals(false);
    m_currentPath = path;
    m_dirty = false;
    updateWindowTitle();
}

bool XmlEditorWindow::saveToPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save"), tr("Cannot write %1:\n%2").arg(path, file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << m_sourceEdit->toPlainText();
    m_currentPath = path;
    m_dirty = false;
    updateWindowTitle();
    return true;
}

void XmlEditorWindow::onNew()
{
    loadSource(QString::fromLatin1(kDefaultTemplate), QString());
    onParse();
}

void XmlEditorWindow::onOpen()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open XML"), QString(), tr("XML files (*.xml);;All files (*)"));
    if (!path.isEmpty())
        openFile(path);
}

bool XmlEditorWindow::onSave()
{
    if (m_currentPath.isEmpty())
        return onSaveAs();
    return saveToPath(m_currentPath);
}

bool XmlEditorWindow::onSaveAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save XML"), m_currentPath, tr("XML files (*.xml);;All files (*)"));
    if (path.isEmpty())
        return false;
    return saveToPath(path);
}

void XmlEditorWindow::onSourceChanged()
{
    m_dirty = true;
    updateWindowTitle();
}

void XmlEditorWindow::onParse()
{
    QList<XmlDiagnostic> diagnostics;
    const bool ok = m_document.parse(m_sourceEdit->toPlainText(), diagnostics);

    m_treeView->clear();
    if (!m_document.root().name.isEmpty()) {
        auto *rootItem = new QTreeWidgetItem(m_treeView);
        rootItem->setText(0, m_document.root().name);
        QStringList attrParts;
        for (const auto &attr : m_document.root().attributes)
            attrParts << QStringLiteral("%1=\"%2\"").arg(attr.first, attr.second);
        rootItem->setText(1, attrParts.join(QStringLiteral(", ")));
        rootItem->setText(2, m_document.root().text.trimmed());
        for (const XmlNode &child : m_document.root().children)
            populateTree(child, rootItem);
        m_treeView->expandAll();
    }

    populateDiagnostics(diagnostics);

    const int errors = std::count_if(diagnostics.cbegin(), diagnostics.cend(),
        [](const XmlDiagnostic &d) { return d.severity == XmlDiagnostic::Severity::Error; });
    const int warnings = diagnostics.size() - errors;

    m_summary->setText(ok
        ? tr("Valid — %1 warning(s)").arg(warnings)
        : tr("Invalid — %1 error(s), %2 warning(s)").arg(errors).arg(warnings));
}

void XmlEditorWindow::populateTree(const XmlNode &node, QTreeWidgetItem *parentItem)
{
    auto *item = new QTreeWidgetItem(parentItem);
    item->setText(0, node.name);

    QStringList attrParts;
    for (const auto &attr : node.attributes)
        attrParts << QStringLiteral("%1=\"%2\"").arg(attr.first, attr.second);
    item->setText(1, attrParts.join(QStringLiteral(", ")));
    item->setText(2, node.text.trimmed());

    for (const XmlNode &child : node.children)
        populateTree(child, item);
}

void XmlEditorWindow::populateDiagnostics(const QList<XmlDiagnostic> &diagnostics)
{
    m_diagTable->setRowCount(diagnostics.size());
    for (int row = 0; row < diagnostics.size(); ++row) {
        const XmlDiagnostic &d = diagnostics.at(row);

        auto *severityItem = new QTableWidgetItem(
            d.severity == XmlDiagnostic::Severity::Error ? tr("Error") : tr("Warning"));
        severityItem->setForeground(d.severity == XmlDiagnostic::Severity::Error
                                         ? QColor(200, 40, 40)
                                         : QColor(180, 130, 0));

        auto *locationItem = new QTableWidgetItem(
            d.line > 0 ? QStringLiteral("%1:%2").arg(d.line).arg(d.column) : QString());

        auto *messageItem = new QTableWidgetItem(d.message);

        m_diagTable->setItem(row, 0, severityItem);
        m_diagTable->setItem(row, 1, locationItem);
        m_diagTable->setItem(row, 2, messageItem);
    }
}

void XmlEditorWindow::updateWindowTitle()
{
    const QString name = m_currentPath.isEmpty() ? tr("untitled.xml") : m_currentPath;
    setWindowTitle(QStringLiteral("%1%2 — libreparser").arg(m_dirty ? QStringLiteral("*") : QString(), name));
}

void XmlEditorWindow::closeEvent(QCloseEvent *event)
{
    if (!m_dirty) {
        event->accept();
        return;
    }
    const auto choice = QMessageBox::question(
        this, tr("Unsaved changes"), tr("Save changes before closing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (choice == QMessageBox::Save) {
        event->setAccepted(onSave());
    } else if (choice == QMessageBox::Discard) {
        event->accept();
    } else {
        event->ignore();
    }
}
