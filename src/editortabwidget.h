// In src/editortabwidget.h

#ifndef EDITORTABWIDGET_H
#define EDITORTABWIDGET_H

#include <QWidget>
// Remove redundant include: #include <QTextEdit> // No longer directly needed if using QPlainTextEdit
#include <QPlainTextEdit>     // Keep this for the actual editor widget
#include <QSyntaxHighlighter> // Base class for highlighters
#include <QFileInfo>          // To extract filename from path

// Forward declarations to avoid circular includes where possible
class LineNumberArea;
class VBSyntaxHighlighter;
class CppSyntaxHighlighter;
class KayteSyntaxHighlighter;
class PascalSyntaxHighlighter;
class DelphiSyntaxHighlighter;

class EditorTabWidget : public QWidget
{
    Q_OBJECT

public:
    // IMPORTANT: Make sure this constructor matches your .cpp implementation
    explicit EditorTabWidget(const QString &filePath = QString(), QWidget *parent = nullptr);
    ~EditorTabWidget();

    // Accessors for content and file path
    QString filePath() const { return m_filePath; }

    // Use m_editor for document access
    bool isModified() const { return m_editor->document()->isModified(); }
    void setModified(bool modified); // Expose this to MainWindow if needed for new tabs

    // This is the correct getter, now returning the consistent m_editor
    QPlainTextEdit* getPlainTextEdit() const { return m_editor; }

    // File operations
    bool loadFile(const QString &filePath);
    bool saveFile(const QString &filePath);

    // Remove textEdit() if you're consistently using QPlainTextEdit
    // If you need a generic QTextEdit* (e.g., for document methods), you can cast or provide another getter:
    // QTextEdit* textEditAsBase() const { return m_editor; } // QPlainTextEdit inherits from QTextEdit

    // Line number area access for update signals (protected/private often fine)
    LineNumberArea *lineNumberArea() const { return m_lineNumberArea; }

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    // Use m_editor's scroll bar for updates
    void updateLineNumberArea(int value);
    void handleContentsChanged(); // Slot for QPlainTextEdit::contentsChanged or document's contentsChanged

    // Remove these if handleContentsChanged is sufficient
    // void on_editor_textChanged();
    // void on_document_contentsChanged();

private:
    QPlainTextEdit *m_editor; // <--- ONLY ONE EDITOR POINTER (QPlainTextEdit*)
    LineNumberArea *m_lineNumberArea;
    QSyntaxHighlighter *m_currentHighlighter; // Pointer to the active highlighter

    // Specific highlighters (owned by this tab)
    VBSyntaxHighlighter *m_vbHighlighter;
    CppSyntaxHighlighter *m_cppHighlighter;
    KayteSyntaxHighlighter *m_kayteHighlighter;
    PascalSyntaxHighlighter *m_pascalHighlighter;
    DelphiSyntaxHighlighter *m_delphiHighlighter;

    QString m_filePath;

    void setupHighlighters(); // Helper to create and manage highlighters
    void applyHighlighterForFile(const QString &filePath);

signals:
    void modificationChanged(bool modified); // Emitted when document modification state changes
    void titleChanged(const QString &newTitle); // Emitted when file path changes (for tab title)
};

#endif // EDITORTABWIDGET_H