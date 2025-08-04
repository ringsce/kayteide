// linenumberarea.h
#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>
#include <QPlainTextEdit> // <--- ADD THIS INCLUDE


// Forward declaration to avoid including mainwindow.h here,
// as LineNumberArea only needs to know about QTextEdit's API.
class QTextEdit;

class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    // This constructor takes a QPlainTextEdit* (the editor it's associated with)
    // AND a QWidget* (its parent).
    explicit LineNumberArea(QPlainTextEdit *editor, QWidget *parent = nullptr);
    // ... other methods ...

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    //QTextEdit *codeEditor;
        QPlainTextEdit *m_codeEditor; // This will hold the pointer to the QPlainTextEdit
};

#endif // LINENUMBERAREA_H