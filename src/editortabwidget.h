#ifndef EDITORTABWIDGET_H
#define EDITORTABWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QVector>

class LineNumberArea;
class VBSyntaxHighlighter;
class CppSyntaxHighlighter;
class KayteSyntaxHighlighter;
class PascalSyntaxHighlighter;
class DelphiSyntaxHighlighter;
class MarkdownSyntaxHighlighter;
class QButtonGroup;
class QSplitter;
class QTextBrowser;
class QTimer;

class EditorTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorTabWidget(const QString &filePath = QString(), QWidget *parent = nullptr);
    ~EditorTabWidget();

    // File operations
    bool loadFile(const QString &filePath);
    bool saveFile(const QString &filePath);

    // Getters
    QString filePath() const { return m_filePath; }
    bool isModified() const;  // ← ADDED: Returns document modified state
    QPlainTextEdit* getPlainTextEdit() const { return m_editor; }
    LineNumberArea* getLineNumberArea() const { return m_lineNumberArea; }

    // Setters
    void setFilePath(const QString &filePath) { m_filePath = filePath; }
    void setModified(bool modified);

    // Build errors / warnings for this file: a bug in the gutter and a
    // tinted line (cleared with an empty list). Lines are 1-based.
    struct Diagnostic {
        int line = 0;
        int column = 0;
        QString severity;   // "error", "warning", "note", …
        QString message;
    };
    void setDiagnostics(const QVector<Diagnostic> &diagnostics);

    // Markdown files get a live preview next to the editor.
    enum class MarkdownView { Editor = 0, Split = 1, Preview = 2 };
    static bool isMarkdownFile(const QString &path);
    bool isMarkdown() const { return m_preview != nullptr; }
    void setMarkdownView(MarkdownView view);

    signals:
        void modificationChanged(bool modified);
    void titleChanged(const QString &newTitle);
    // A link in the Markdown preview points at a local file.
    void openFileRequested(const QString &path);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(int value);
    void handleContentsChanged();

private:
    void setupHighlighters();
    void applyHighlighterForFile(const QString &filePath);
    void finishLoad(const QString &filePath, const QString &content);
    void updateExtraSelections();   // current line + diagnostic lines
    void updateMarkdownMode();   // adds / removes the preview for m_filePath
    void renderPreview();
    void stylePreview();         // code / quote / heading touches after a render
    void syncPreviewScroll();
    void onPreviewLinkClicked(const QUrl &url);

    QPlainTextEdit *m_editor;
    LineNumberArea *m_lineNumberArea;

    // Syntax highlighters
    QSyntaxHighlighter *m_currentHighlighter;
    VBSyntaxHighlighter *m_vbHighlighter;
    CppSyntaxHighlighter *m_cppHighlighter;
    KayteSyntaxHighlighter *m_kayteHighlighter;
    PascalSyntaxHighlighter *m_pascalHighlighter;
    DelphiSyntaxHighlighter *m_delphiHighlighter;
    MarkdownSyntaxHighlighter *m_markdownHighlighter { nullptr };

    // Markdown preview (created on demand for .md files)
    QWidget      *m_editorPane   { nullptr };   // line numbers + editor
    QSplitter    *m_splitter     { nullptr };
    QWidget      *m_markdownBar  { nullptr };
    QButtonGroup *m_viewButtons  { nullptr };
    QTextBrowser *m_preview      { nullptr };
    QTimer       *m_previewTimer { nullptr };

    QString m_filePath;
    QVector<Diagnostic> m_diagnostics;
};

#endif // EDITORTABWIDGET_H