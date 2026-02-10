#include "editortabwidget.h"
#include "linenumberarea.h" // Make sure this is correctly implemented
#include "vbsyntaxhighlighter.h"
#include "cppsyntaxhighlighter.h"
#include "kaytesyntaxhighlighter.h"
#include "pascalsyntaxhighlighter.h"
#include "delphisyntaxhighlighter.h"


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QTextCharFormat>
#include <QDebug>
#include <QFileInfo> // Added for QFileInfo::suffix()

EditorTabWidget::EditorTabWidget(const QString &filePath, QWidget *parent)
    : QWidget(parent),
      // IMPORTANT: Initialize m_editor (QPlainTextEdit) FIRST
      m_editor(new QPlainTextEdit(this)), // m_editor is a QPlainTextEdit*
      // Now, correctly initialize m_lineNumberArea by passing m_editor AND this
      m_lineNumberArea(new LineNumberArea(m_editor, this)),
      m_currentHighlighter(nullptr),
      m_vbHighlighter(nullptr),
      m_cppHighlighter(nullptr),
      m_kayteHighlighter(nullptr),
      m_pascalHighlighter(nullptr),
      m_delphiHighlighter(nullptr),
      m_filePath(filePath)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create a horizontal layout to hold both the line number area and the text editor
    QHBoxLayout *editorLayout = new QHBoxLayout();
    editorLayout->setContentsMargins(0,0,0,0);
    editorLayout->setSpacing(0); // No space between line number area and editor

    editorLayout->addWidget(m_lineNumberArea); // Add line number area first
    editorLayout->addWidget(m_editor);         // Then add the editor

    layout->addLayout(editorLayout); // Add the combined layout to the main vertical layout

    // Connect signals from m_editor to line number area and other updates
    connect(m_editor, &QPlainTextEdit::blockCountChanged, this, &EditorTabWidget::updateLineNumberAreaWidth);
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, m_lineNumberArea, QOverload<>::of(&QWidget::update)); // Connect to scroll updates directly
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &EditorTabWidget::highlightCurrentLine);
    connect(m_editor->document(), &QTextDocument::contentsChanged, this, &EditorTabWidget::handleContentsChanged);

    setupHighlighters();

    if (!m_filePath.isEmpty()) {
        // Use the loadFile method to load content and apply highlighter
        loadFile(m_filePath);
    } else {
        m_editor->setPlainText("");
        m_editor->document()->setModified(false);
        emit titleChanged(tr("Untitled"));
    }

    highlightCurrentLine();
    updateLineNumberAreaWidth(m_editor->blockCount()); // Ensure initial width is set
}

EditorTabWidget::~EditorTabWidget()
{
    // Highlighters are parented to QTextDocument. When m_editor->document() is deleted,
    // it will automatically delete its child highlighters. Explicit deletion is not needed here.
    qDebug() << "EditorTabWidget destroyed for file:" << m_filePath;
}

bool EditorTabWidget::loadFile(const QString &filePath) {
    qDebug() << "EditorTabWidget::loadFile called for:" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_editor->setPlainText(tr("Could not open file: %1\n%2").arg(filePath, file.errorString()));
        setModified(false); // Not modified
        return false;
    }

    // Load the file content
    QString content = file.readAll();
    file.close();

    qDebug() << "Loaded" << content.length() << "characters from" << filePath;

    // Set the content
    m_editor->setPlainText(content);
    m_filePath = filePath;

    // CRITICAL: Clear modified flag AFTER setting text
    m_editor->document()->setModified(false);

    // Update line number area for the new content
    if (m_lineNumberArea) {
        qDebug() << "Updating line number area, block count:" << m_editor->blockCount();
        m_lineNumberArea->setupConnections();
        m_lineNumberArea->updateGeometry();
        m_lineNumberArea->update();
    } else {
        qWarning() << "Line number area is null!";
    }

    // Apply syntax highlighter based on file extension
    applyHighlighterForFile(m_filePath);

    // Emit signals
    emit titleChanged(QFileInfo(m_filePath).fileName());
    emit modificationChanged(false);

    qDebug() << "File loaded successfully, modified state:" << m_editor->document()->isModified();

    return true;
}

void EditorTabWidget::setupHighlighters()
{
    // Ensure m_editor is valid before trying to access its document
    if (!m_editor || !m_editor->document()) {
        qWarning() << "EditorTabWidget::setupHighlighters: Editor or document not available.";
        return;
    }

    QTextDocument *doc = m_editor->document(); // <--- Changed m_textEdit to m_editor

    // Initialize your highlighters with the document
    // Ensure you have included the headers for your highlighters
    m_vbHighlighter = new VBSyntaxHighlighter(doc);
    m_cppHighlighter = new CppSyntaxHighlighter(doc);
    m_kayteHighlighter = new KayteSyntaxHighlighter(doc);
    m_pascalHighlighter = new PascalSyntaxHighlighter(doc);
    m_delphiHighlighter = new DelphiSyntaxHighlighter(doc);

    m_currentHighlighter = nullptr;
}

// --- REMOVED THE FOLLOWING DUPLICATE/OBSOLETE FUNCTIONS ---
// void EditorTabWidget::on_document_contentsChanged() { /* ... */ }
// void EditorTabWidget::on_editor_textChanged() { /* ... */ }
// --- END REMOVAL ---


void EditorTabWidget::applyHighlighterForFile(const QString &filePath)
{
    if (m_currentHighlighter) {
        m_currentHighlighter->setDocument(nullptr);
    }

    QString suffix = QFileInfo(filePath).suffix().toLower();
    QTextDocument *doc = m_editor->document(); // <--- Changed m_textEdit to m_editor

    if (suffix == "vb") {
        m_currentHighlighter = m_vbHighlighter;
    } else if (suffix == "cpp" || suffix == "h" || suffix == "cxx" || suffix == "hpp") { // Added more C++ extensions
        m_currentHighlighter = m_cppHighlighter;
    } else if (suffix == "kayte" || suffix == "kyt") {
        m_currentHighlighter = m_kayteHighlighter;
    } else if (suffix == "pas" || suffix == "pp" || suffix == "dpr") {
        m_currentHighlighter = m_pascalHighlighter;
    } else if (suffix == "dfm") {
        m_currentHighlighter = m_delphiHighlighter;
    } else {
        m_currentHighlighter = nullptr;
    }

    if (m_currentHighlighter) {
        m_currentHighlighter->setDocument(doc); // Attach new highlighter
        m_currentHighlighter->rehighlight(); // Re-apply highlighting
    }
}

bool EditorTabWidget::isModified() const
{
    return m_editor && m_editor->document() && m_editor->document()->isModified();
}

void EditorTabWidget::setModified(bool modified)
{
    if (!m_editor || !m_editor->document()) {
        qWarning() << "EditorTabWidget::setModified: Editor or document not available";
        return;
    }

    bool currentState = m_editor->document()->isModified();

    qDebug() << "setModified called: current =" << currentState << ", new =" << modified;

    // Only update and emit if the modification state actually changes
    if (currentState != modified) {
        m_editor->document()->setModified(modified);
        emit modificationChanged(modified);
        qDebug() << "Modification state changed to:" << modified;
    }
}

// NOTE: There was a duplicate loadFile function. I've kept the one from the top
// and removed this one, as the constructor now correctly calls the top one.
/*
bool EditorTabWidget::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file %1: %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream in(&file);
    m_textEdit->setText(in.readAll()); // <--- This would also need to be m_editor
    file.close();

    m_filePath = filePath;
    m_textEdit->document()->setModified(false); // <--- This would also need to be m_editor

    applyHighlighterForFile(m_filePath);

    emit modificationChanged(false);
    emit titleChanged(QFileInfo(m_filePath).fileName());

    return true;
}
*/

bool EditorTabWidget::saveFile(const QString &filePath)
{
    qDebug() << "EditorTabWidget::saveFile called for:" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save file %1: %2").arg(filePath, file.errorString()));
        qDebug() << "Save failed:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    file.close();

    // Update file path if it changed (e.g., Save As)
    if (m_filePath != filePath) {
        m_filePath = filePath;
        applyHighlighterForFile(m_filePath);
        emit titleChanged(QFileInfo(m_filePath).fileName());
    }

    // CRITICAL: Clear modified flag after successful save
    m_editor->document()->setModified(false);

    // Emit modification state changed
    emit modificationChanged(false);

    qDebug() << "File saved successfully, modified state:" << m_editor->document()->isModified();

    return true;
}

void EditorTabWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(cr.x(), cr.y(), m_lineNumberArea->sizeHint().width(), cr.height());
    // m_editor's size is managed by the QHBoxLayout, no explicit setGeometry needed for it.
}

void EditorTabWidget::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    if (m_lineNumberArea) {
        m_lineNumberArea->setFixedWidth(m_lineNumberArea->sizeHint().width());
    }
}

void EditorTabWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!m_editor->isReadOnly()) { // <--- Changed m_textEdit to m_editor
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = m_editor->textCursor(); // <--- Changed m_textEdit to m_editor
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    m_editor->setExtraSelections(extraSelections); // <--- Changed m_textEdit to m_editor
}

void EditorTabWidget::updateLineNumberArea(int /*value*/)
{
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void EditorTabWidget::handleContentsChanged()
{
    if (!m_editor || !m_editor->document()) {
        return;
    }

    bool modified = m_editor->document()->isModified();
    qDebug() << "Content changed, document modified:" << modified;
    emit modificationChanged(modified);
}