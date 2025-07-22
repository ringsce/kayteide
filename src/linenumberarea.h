// linenumberarea.h
#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>

// Forward declaration to avoid including mainwindow.h here,
// as LineNumberArea only needs to know about QTextEdit's API.
class QTextEdit;

class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    explicit LineNumberArea(QTextEdit *editor, QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTextEdit *codeEditor;
};

#endif // LINENUMBERAREA_H