// mainwindow.h (UPDATED)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSyntaxHighlighter>

#include "ui_mainwindow.h" // Crucial for Ui::MainWindow definition

// Forward declarations for your highlighters
class VBSyntaxHighlighter;
class CppSyntaxHighlighter;
class KayteSyntaxHighlighter;
class PascalSyntaxHighlighter;
class DelphiSyntaxHighlighter;

// Forward declare LineNumberArea
class LineNumberArea; // <--- ADD THIS

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void saveFile();
    void saveFileAs();
    void buildProject();
    void runProject();
    void cleanProject();
    void debugProject();
    void showAboutDialog();

    // New Slot for line number area updates
    void updateLineNumberAreaWidth(int newBlockCount); // <--- ADD THIS
    void updateLineNumberArea(const QRect &rect, int dy); // <--- ADD THIS (for scrolling)

private:
    Ui::MainWindow *ui;
    QString currentFile;

    VBSyntaxHighlighter *vbHighlighter;
    CppSyntaxHighlighter *cppHighlighter;
    KayteSyntaxHighlighter *kayteHighlighter;
    PascalSyntaxHighlighter *pascalHighlighter;
    DelphiSyntaxHighlighter *delphiHighlighter;

    QSyntaxHighlighter *currentHighlighter;

    LineNumberArea *lineNumberArea; // <--- ADD THIS

    void setHighlighter(QSyntaxHighlighter *newHighlighter);

protected:
    void resizeEvent(QResizeEvent *event) override; // <--- ADD THIS LINE

};
#endif // MAINWINDOW_H