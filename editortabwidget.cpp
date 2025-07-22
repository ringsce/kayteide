#include "editortabwidget.h"
#include "linenumberarea.h" // Make sure this is correctly implemented
#include "vbsyntaxhighlighter.h" // <--- ADD THIS
#include "cppsyntaxhighlighter.h" // <--- ADD THIS
#include "kaytesyntaxhighlighter.h" // <--- ADD THIS
#include "pascalsyntaxhighlighter.h" // <--- ADD THIS
#include "delphisyntaxhighlighter.h" // <--- ADD THIS

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFontDatabase> // For monospace font
#include <QGuiApplication> // For palette access
#include <QTextCharFormat> // For current line highlighting
#include <QDebug> // For debugging output

EditorTabWidget::EditorTabWidget(QWidget *parent)
    : QWidget(parent)
    , m_textEdit(new QTextEdit(this)) // Initialize QTextEdit
    , m_lineNumberArea(new LineNumberArea(m_textEdit)) // Initialize LineNumberArea, passing QTextEdit
    , m_currentHighlighter(nullptr) // Initialize current highlighter to null
    , m_filePath("") // Initialize file path to empty
{
    // --- Layout setup ---
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0); // No margins for tight fit
    hLayout->setSpacing(0); // No spacing between line number area and text edit

    hLayout->addWidget(m_lineNumberArea);
    hLayout->addWidget(m_textEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(hLayout);

    // Set a monospace font for coding
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(12);
    m_textEdit->setFont(font);
    m_textEdit->setAcceptRichText(false); // Only plain text for code editor

    // --- Connections ---
    // Connect textEdit signals to line number area updates
    connect(m_textEdit->document(), &QTextDocument::blockCountChanged,
            this, &EditorTabWidget::updateLineNumberAreaWidth);
    connect(m_textEdit->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &EditorTabWidget::updateLineNumberArea);
    connect(m_textEdit, &QTextEdit::textChanged,
            this, &EditorTabWidget::handleContentsChanged); // For modification tracking
    connect(m_textEdit, &QTextEdit::cursorPositionChanged,
            this, &EditorTabWidget::highlightCurrentLine);

    // Initial update for line number area width
    updateLineNumberAreaWidth(m_textEdit->document()->blockCount());
    highlightCurrentLine(); // Highlight current line on startup

    // --- Syntax Highlighters setup ---
    setupHighlighters();
}

EditorTabWidget::~EditorTabWidget()
{
    // QObject's parent-child mechanism will delete m_textEdit and m_lineNumberArea
    // (since they were created with 'this' as parent).
    // Highlighters are explicitly deleted because they are not parented to 'this'
    // but rather to m_textEdit->document(). When the document is deleted, highlighters
    // associated with it are usually also deleted. However, explicitly nulling and
    // deleting them ensures proper cleanup if they outlive the document somehow.
    // For QSyntaxHighlighter, its parent is the QTextDocument. When the document
    // is deleted, it will delete its child highlighters. So explicit deletes are
    // often not strictly necessary here, but can be a safeguard depending on ownership.

    // A safer pattern is to ensure highlighters are parented to the document.
    // When the document is deleted, it deletes its children.
    // In setupHighlighters, we set their parent to m_textEdit->document().
    // So, no explicit deletion is needed here.
    // delete m_vbHighlighter; // No need if parented correctly
    // ...
    qDebug() << "EditorTabWidget destroyed for file:" << m_filePath;
}

void EditorTabWidget::setupHighlighters()
{
    QTextDocument *doc = m_textEdit->document();
    m_vbHighlighter = new VBSyntaxHighlighter(doc);
    m_cppHighlighter = new CppSyntaxHighlighter(doc);
    m_kayteHighlighter = new KayteSyntaxHighlighter(doc);
    m_pascalHighlighter = new PascalSyntaxHighlighter(doc);
    m_delphiHighlighter = new DelphiSyntaxHighlighter(doc);

    // Initially, no highlighter is active until a file type is determined
    // m_currentHighlighter will be set by applyHighlighterForFile
    m_currentHighlighter = nullptr;
}

void EditorTabWidget::applyHighlighterForFile(const QString &filePath)
{
    // Disable any currently active highlighter
    if (m_currentHighlighter) {
        m_currentHighlighter->setDocument(nullptr); // Detach from document
        m_currentHighlighter = nullptr;
    }

    QString suffix = QFileInfo(filePath).suffix().toLower();
    QTextDocument *doc = m_textEdit->document();

    if (suffix == "vb") {
        m_currentHighlighter = m_vbHighlighter;
    } else if (suffix == "cpp" || suffix == "h") {
        m_currentHighlighter = m_vbHighlighter;
    } else if (suffix == "kayte" || suffix == "kyt") {
        m_currentHighlighter = m_kayteHighlighter;
    } else if (suffix == "pas" || suffix == "pp" || suffix == "dpr") {
        m_currentHighlighter = m_pascalHighlighter;
    }
    // else if for Delphi? if (suffix == "dfm") { m_currentHighlighter = m_delphiHighlighter; }
    // You might want a default plain text highlighter or none for unknown types.

    if (m_currentHighlighter) {
        m_currentHighlighter->setDocument(doc); // Attach new highlighter
        m_currentHighlighter->rehighlight(); // Re-apply highlighting
    }
}

void EditorTabWidget::setModified(bool modified)
{
    if (m_textEdit->document()->isModified() != modified) {
        m_textEdit->document()->setModified(modified);
        emit modificationChanged(modified); // Emit signal if state actually changes
    }
}

bool EditorTabWidget::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file %1: %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream in(&file);
    m_textEdit->setText(in.readAll());
    file.close();

    m_filePath = filePath; // Store the file path
    m_textEdit->document()->setModified(false); // File is now saved

    // Apply highlighter based on new file path
    applyHighlighterForFile(m_filePath);

    // Emit signals for MainWindow to update tab title
    emit modificationChanged(false); // Explicitly state not modified after load
    emit titleChanged(QFileInfo(m_filePath).fileName());

    return true;
}

bool EditorTabWidget::saveFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save file %1: %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << m_textEdit->toPlainText();
    file.close();

    m_filePath = filePath; // Update path in case of Save As
    m_textEdit->document()->setModified(false); // File is now saved

    // Apply highlighter again in case file type changed (e.g. from Untitled to .cpp)
    applyHighlighterForFile(m_filePath);

    // Emit signals for MainWindow to update tab title
    emit modificationChanged(false); // Explicitly state not modified after save
    emit titleChanged(QFileInfo(m_filePath).fileName());

    return true;
}

void EditorTabWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event); // Call base class implementation

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(cr.x(), cr.y(), m_lineNumberArea->sizeHint().width(), cr.height());
    // The m_textEdit is handled by the QHBoxLayout, so no explicit setGeometry needed for it.
}

void EditorTabWidget::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    // The LineNumberArea's sizeHint() is dynamic based on block count.
    // Just update its fixed width based on its size hint.
    if (m_lineNumberArea) {
        m_lineNumberArea->setFixedWidth(m_lineNumberArea->sizeHint().width());
    }
}

void EditorTabWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!m_textEdit->isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(160); // Light yellow

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = m_textEdit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    m_textEdit->setExtraSelections(extraSelections);
}


// Before:
// void EditorTabWidget::updateLineNumberArea(const QRect &rect, int dy)
// After:
void EditorTabWidget::updateLineNumberArea(int /*value*/) // <--- CHANGE THIS
{
    // Now, when the scrollbar moves, we just tell the line number area to repaint itself fully.
    // The LineNumberArea internally needs to know the textEdit's scroll position to draw correctly.
    if (m_lineNumberArea) {
        m_lineNumberArea->update(); // Just trigger a repaint of the line number area
    }
}

void EditorTabWidget::handleContentsChanged()
{
    // This slot is connected to QTextEdit::textChanged.
    // QTextDocument::isModified() will already be true at this point.
    // Emit the signal to notify MainWindow about modification state.
    emit modificationChanged(m_textEdit->document()->isModified());
}