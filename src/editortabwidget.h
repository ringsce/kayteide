#ifndef EDITORTABWIDGET_H
#define EDITORTABWIDGET_H

#include <QWidget>
#include <QTextEdit>
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
    explicit EditorTabWidget(QWidget *parent = nullptr);
    ~EditorTabWidget();

    // Accessors for content and file path
    QString filePath() const { return m_filePath; }
    bool isModified() const { return m_textEdit->document()->isModified(); }
    void setModified(bool modified); // Expose this to MainWindow if needed for new tabs

    // File operations
    bool loadFile(const QString &filePath);
    bool saveFile(const QString &filePath);

    // Get the QTextEdit for external access (e.g., for global find/replace if needed)
    QTextEdit* textEdit() const { return m_textEdit; }

    // Line number area access for update signals (protected/private often fine)
    LineNumberArea *lineNumberArea() const { return m_lineNumberArea; }

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(int value); // <--- CHANGE THIS LINE IN THE HEADER
    void handleContentsChanged(); // Slot for QTextEdit::contentsChanged

private:
    QTextEdit *m_textEdit;
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