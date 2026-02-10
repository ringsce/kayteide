#ifndef EDITORTABWIDGET_H
#define EDITORTABWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>

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

    signals:
        void modificationChanged(bool modified);
    void titleChanged(const QString &newTitle);

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

    QPlainTextEdit *m_editor;
    LineNumberArea *m_lineNumberArea;

    // Syntax highlighters
    QSyntaxHighlighter *m_currentHighlighter;
    VBSyntaxHighlighter *m_vbHighlighter;
    CppSyntaxHighlighter *m_cppHighlighter;
    KayteSyntaxHighlighter *m_kayteHighlighter;
    PascalSyntaxHighlighter *m_pascalHighlighter;
    DelphiSyntaxHighlighter *m_delphiHighlighter;

    QString m_filePath;
};

#endif // EDITORTABWIDGET_H