#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>
#include <QPlainTextEdit>

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(QPlainTextEdit *editor, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    // Method to set or update the code editor
    void setCodeEditor(QPlainTextEdit *editor);

    // Method to reconnect signals (useful when document changes)
    void setupConnections();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onDocumentChanged();
    void onBlockCountChanged(int newBlockCount);
    void updateArea(const QRect &rect, int dy);

private:
    QPlainTextEdit *m_codeEditor;
};

#endif // LINENUMBERAREA_H