#include "mainwindow.h"

// Your includes
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QIcon>
#include <QFileInfo>
#include <QPixmap>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFont>
#include <QSizeF> // Still good to have this if QAbstractTextDocumentLayout uses QSizeF

#include "linenumberarea.h"
#include "vbsyntaxhighlighter.h"
#include "cppsyntaxhighlighter.h"
#include "kaytesyntaxhighlighter.h"
#include "pascalsyntaxhighlighter.h"
#include "delphisyntaxhighlighter.h"

// This is the ONLY definition of the MainWindow constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentHighlighter(nullptr)
{
    ui->setupUi(this); // This line creates ui->centralwidget and its initial layout

    setWindowTitle("Kayte IDE"); // Set window title early

    QFont font("Menlo");
    font.setPointSize(12);
    font.setStyleHint(QFont::Monospace);
    ui->memo1->setFont(font);


    // --- Line Number Area Integration (FINAL REVISED VERSION) ---

    // 1. Create a new widget to act as a container for your editor and line numbers.
    // Parent it to 'this' (the MainWindow) so it gets deleted when MainWindow does.
    QWidget *editorContainerWidget = new QWidget(this);

    // 2. Create the horizontal layout for this new container widget.
    QHBoxLayout *editorContentLayout = new QHBoxLayout(editorContainerWidget);
    editorContentLayout->setContentsMargins(0, 0, 0, 0); // No extra margins
    editorContentLayout->setSpacing(0); // No spacing between line numbers and text edit

    // 3. IMPORTANT: Reparent ui->memo1 from its default parent (ui->centralwidget)
    // to our new editorContainerWidget. This must happen before adding it to the new layout.
    ui->memo1->setParent(editorContainerWidget);

    // 4. Create the LineNumberArea, passing ui->memo1 as its associated editor.
    // Parent it to editorContainerWidget.
    lineNumberArea = new LineNumberArea(ui->memo1, editorContainerWidget);

    // 5. Add the line number area and the reparented text edit to the new editorContentLayout.
    editorContentLayout->addWidget(lineNumberArea);
    editorContentLayout->addWidget(ui->memo1);

    // 6. Replace the MainWindow's central widget with our new editorContainerWidget.
    // This correctly integrates your custom layout into the main window.
    // The old central widget created by ui->setupUi will be implicitly deleted
    // when setCentralWidget replaces it, as long as it was a direct child of 'this'.
    setCentralWidget(editorContainerWidget);

    // Connect signals for line number area updates
    connect(ui->memo1->document(), &QTextDocument::blockCountChanged, this, &MainWindow::updateLineNumberAreaWidth);

    connect(ui->memo1->verticalScrollBar(), QOverload<int>::of(&QScrollBar::valueChanged),
            lineNumberArea, [this]() {
        lineNumberArea->update();
    });

    connect(ui->memo1->document()->documentLayout(), QOverload<const QSizeF&>::of(&QAbstractTextDocumentLayout::documentSizeChanged),
            lineNumberArea, [this]() {
        lineNumberArea->update();
    });

    // --- Initial setup for the LineNumberArea width ---
    updateLineNumberAreaWidth(ui->memo1->document()->blockCount());


    // --- Rest of your MainWindow constructor code (unchanged from here) ---
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open"));
    ui->actionSave->setIcon(QIcon::fromTheme("document-save"));
    ui->actionSave_As->setIcon(QIcon::fromTheme("document-save-as"));
    ui->actionExit->setIcon(QIcon::fromTheme("application-exit"));

    ui->actionBuild->setIcon(QIcon::fromTheme("system-run"));
    ui->actionClean->setIcon(QIcon::fromTheme("edit-clear"));
    ui->actionRun->setIcon(QIcon::fromTheme("media-playback-start"));
    ui->actionDebug->setIcon(QIcon::fromTheme("tools-debugger"));

    ui->actionAbout->setIcon(QIcon::fromTheme("help-about"));

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionBuild, &QAction::triggered, this, &MainWindow::buildProject);
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::runProject);
    connect(ui->actionClean, &QAction::triggered, this, &MainWindow::cleanProject);
    connect(ui->actionDebug, &QAction::triggered, this, &MainWindow::debugProject);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);

    vbHighlighter = new VBSyntaxHighlighter(ui->memo1->document());
    cppHighlighter = new CppSyntaxHighlighter(ui->memo1->document());
    kayteHighlighter = new KayteSyntaxHighlighter(ui->memo1->document());
    pascalHighlighter = new PascalSyntaxHighlighter(ui->memo1->document());
    delphiHighlighter = new DelphiSyntaxHighlighter(ui->memo1->document());

    setHighlighter(kayteHighlighter);
}


// Ensure the destructor is also defined only once
MainWindow::~MainWindow()
{
    if (vbHighlighter) delete vbHighlighter;
    if (cppHighlighter) delete cppHighlighter;
    if (kayteHighlighter) delete kayteHighlighter;
    if (pascalHighlighter) delete pascalHighlighter;
    if (delphiHighlighter) delete delphiHighlighter;

    // You also need to delete lineNumberArea if it's not parented correctly to be auto-deleted
    // If lineNumberArea's parent is 'this' (the MainWindow), it will be deleted by MainWindow's destructor.
    // If it's parented to `centralWidget`, and centralWidget is deleted, it will also be deleted.
    // So, an explicit `delete lineNumberArea;` is usually not necessary here if parenting is correct.
    // However, if you explicitly created `editorContainer` as a new QWidget and set it as central,
    // then `delete editorContainer;` would be needed, and that would delete its children (`lineNumberArea` and `memo1`).

    delete ui; // Always delete the UI object created by setupUi
}

// All other member functions (slots, setHighlighter, etc.) go here, defined only once.
void MainWindow::setHighlighter(QSyntaxHighlighter *newHighlighter)
{
    if (currentHighlighter == newHighlighter) {
        return;
    }

    if (currentHighlighter) {
        currentHighlighter->setDocument(nullptr);
    }

    currentHighlighter = newHighlighter;

    if (currentHighlighter) {
        currentHighlighter->setDocument(ui->memo1->document());
        currentHighlighter->rehighlight();
    }
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(this, "Open File");
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->memo1->setPlainText(file.readAll());
            currentFile = path;

            QFileInfo fileInfo(path);
            QString suffix = fileInfo.suffix().toLower();

            if (suffix == "vb" || suffix == "bas" || suffix == "frm" || suffix == "cls") {
                setHighlighter(vbHighlighter);
            } else if (suffix == "cpp" || suffix == "cxx" || suffix == "cc" || suffix == "h" || suffix == "hpp") {
                setHighlighter(cppHighlighter);
            } else if (suffix == "kayte" || suffix == "kyt") {
                setHighlighter(kayteHighlighter);
            } else if (suffix == "pas" || suffix == "pp") {
                setHighlighter(pascalHighlighter);
            } else if (suffix == "dpr" || suffix == "pas") {
                setHighlighter(delphiHighlighter);
            } else {
                setHighlighter(nullptr);
            }

        } else {
            QMessageBox::warning(this, "Error", "Could not open file: " + file.errorString());
        }
    }
}

void MainWindow::saveFile()
{
    if (currentFile.isEmpty()) {
        saveFileAs();
    } else {
        QFile file(currentFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << ui->memo1->toPlainText();
        } else {
            QMessageBox::warning(this, "Error", "Could not save file: " + file.errorString());
        }
    }
}

void MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(this, "Save File As");
    if (!path.isEmpty()) {
        currentFile = path;
        saveFile();
    }
}

// Placeholder actions
void MainWindow::buildProject()
{
    QMessageBox::information(this, "Build", "Build project action triggered.");
}

void MainWindow::runProject()
{
    QMessageBox::information(this, "Run", "Run project action triggered.");
}

void MainWindow::cleanProject()
{
    QMessageBox::information(this, "Clean", "Clean project action triggered.");
}

void MainWindow::debugProject()
{
    QMessageBox::information(this, "Debug", "Debug project action triggered.");
}

void MainWindow::showAboutDialog()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About Kayte IDE");
    aboutBox.setIconPixmap(QIcon::fromTheme("help-about").pixmap(64, 64));
    aboutBox.setTextFormat(Qt::RichText);

    QString aboutText = "<b>Kayte IDE</b><br>"
                        "Version 1.0 (Alpha)<br><br>"
                        "A lightweight, cross-platform Integrated Development Environment.<br>"
                        "Built with Qt 6 and C++17.<br><br>"
                        "&copy; Copyright 2025 Pedro Dias Vicente.<br>"
                        "All rights reserved.";

    aboutBox.setText(aboutText);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

// Implement the resizeEvent
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event); // Call base class implementation

    // Get the geometry of the QTextEdit's viewport
    // The viewport is the actual area where text is drawn,
    // excluding scrollbars and the frame.
    // LineNumberArea should align with this viewport.
    QRect cr = ui->memo1->contentsRect(); // This gives the area available for children within the QScrollArea's frame.

    // Position the line number area
    // x: 0 (left edge of the central widget's layout)
    // y: top of the memo1's content rectangle
    // width: calculated by LineNumberArea::sizeHint()
    // height: height of the memo1's content rectangle
    lineNumberArea->setGeometry(cr.x(), cr.y(), lineNumberArea->sizeHint().width(), cr.height());

    // Also, trigger an update for the line number area
    lineNumberArea->update();
}

// --- SLOTS FOR LINE NUMBER AREA ---
// Ensure updateLineNumberAreaWidth still calls setFixedWidth to adjust the layout's distribution
void MainWindow::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    lineNumberArea->setFixedWidth(lineNumberArea->sizeHint().width());
    lineNumberArea->update();
}

// This slot might still be useful for fine-grained updates, but the paintEvent in LineNumberArea
// needs to properly convert coordinates.
void MainWindow::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
}