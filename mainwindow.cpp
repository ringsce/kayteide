#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt Includes
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QIcon>
#include <QFileInfo>
#include <QPixmap>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QDir>
#include <QCloseEvent>
#include <QDebug> // For debugging messages

// Your custom widget and dialog includes
#include "editortabwidget.h"
#include "choicemode.h"
#include "downloadprogressdialog.h"

// This is the ONLY definition of the MainWindow constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this); // This line creates ui->centralwidget and its initial layout

    setWindowTitle(tr("Kayte IDE")); // Set window title early, using tr() for i18n

    // --- Tab Widget Integration ---
    // Assuming 'ui->tabWidgetEditor' is the QTabWidget added in mainwindow.ui
    // And it's set as the central widget of MainWindow.
    setCentralWidget(ui->tabWidgetEditor); // IMPORTANT: Set the QTabWidget as central widget

    // Enable the close button on each tab (the 'x')
    ui->tabWidgetEditor->setTabsClosable(true);
    ui->tabWidgetEditor->setMovable(true); // Allow reordering tabs

    // Connect tab close requested signal (the 'x' button on tabs)
    connect(ui->tabWidgetEditor, &QTabWidget::tabCloseRequested,
            this, &MainWindow::on_tabWidgetEditor_tabCloseRequested);

    // Create an initial empty tab on startup
    createNewTab();

    // --- Global Setup (not tab-specific) ---
    // Set up default download path (e.g., in user's documents/KayteIDE_Resources)
    defaultDownloadPath = QDir::homePath() + QDir::separator() + "KayteIDE_Resources";
    QDir().mkpath(defaultDownloadPath); // Ensure the directory exists

    // Populate the lists of repositories
    setupDownloadRepos(); // Call this once in the constructor

    // --- Actions, Icons, and Connections (for Menu/Toolbar) ---
    ui->actionNewFile->setIcon(QIcon::fromTheme("document-new"));
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open"));
    ui->actionSave->setIcon(QIcon::fromTheme("document-save"));
    ui->actionSave_As->setIcon(QIcon::fromTheme("document-save-as"));
    ui->actionCloseTab->setIcon(QIcon::fromTheme("tab-close"));
    ui->actionExit->setIcon(QIcon::fromTheme("application-exit"));

    ui->actionBuild->setIcon(QIcon::fromTheme("system-run"));
    ui->actionClean->setIcon(QIcon::fromTheme("edit-clear"));
    ui->actionRun->setIcon(QIcon::fromTheme("media-playback-start"));
    ui->actionDebug->setIcon(QIcon::fromTheme("tools-debugger"));

    ui->actionAbout->setIcon(QIcon::fromTheme("help-about"));

    // Connect menu actions to the new tab-aware slots
    connect(ui->actionNewFile, &QAction::triggered, this, &MainWindow::on_actionNewFile_triggered);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::on_actionOpenFile_triggered);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::on_actionSaveFile_triggered);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::on_actionSaveFileAs_triggered);
    connect(ui->actionCloseTab, &QAction::triggered, this, &MainWindow::on_actionCloseTab_triggered);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionBuild, &QAction::triggered, this, &MainWindow::buildProject);
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::runProject);
    connect(ui->actionClean, &QAction::triggered, this, &MainWindow::cleanProject);
    connect(ui->actionDebug, &QAction::triggered, this, &MainWindow::debugProject);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);

    // Show the mode selection dialog on startup
    // Using Qt::QueuedConnection to ensure UI is fully set up before dialog appears
    QMetaObject::invokeMethod(this, &MainWindow::showModeSelectionDialog, Qt::QueuedConnection);
}

// Ensure the destructor is also defined only once
MainWindow::~MainWindow()
{
    // openEditorTabs vector holds pointers to EditorTabWidget.
    // When ui->tabWidgetEditor is deleted (by delete ui;), it deletes its child widgets (EditorTabWidget instances).
    // The EditorTabWidget's destructor will then clean up its own highlighters and LineNumberArea.
    // So, no explicit deletion of individual highlighters or lineNumberArea is needed here.
    delete ui; // Always delete the UI object created by setupUi
}

EditorTabWidget* MainWindow::currentEditorTab() const
{
    // currentWidget() returns the QWidget associated with the current tab
    return qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->currentWidget());
}

void MainWindow::createNewTab(const QString &filePath)
{
    // If opening a new, untitled file, check if the current tab is an unsaved "Untitled" file.
    // This prevents opening multiple "Untitled*" tabs without a save prompt.
    if (filePath.isEmpty() && ui->tabWidgetEditor->count() > 0) { // Check if any tabs exist
        EditorTabWidget* current = currentEditorTab();
        if (current && current->filePath().isEmpty() && current->isModified()) {
            // Prompt to save current untitled tab
            if (!saveCurrentFile()) {
                return; // User cancelled saving, so don't create a new tab
            }
        }
    }

    EditorTabWidget *editorTab = new EditorTabWidget(this); // 'this' (MainWindow) is parent
    openEditorTabs.append(editorTab); // Keep track of it in our vector

    // Connect modification changed signal to update tab title (e.g., add/remove '*')
    connect(editorTab, &EditorTabWidget::modificationChanged,
            this, &MainWindow::updateTabTitle);
    // Connect title changed signal to update tab title (e.g., after Save As)
    connect(editorTab, &EditorTabWidget::titleChanged,
            this, &MainWindow::updateTabTitleOnRename);

    QString tabTitle = tr("Untitled");
    bool fileLoadedSuccessfully = false;

    if (!filePath.isEmpty()) {
        fileLoadedSuccessfully = editorTab->loadFile(filePath);
        if (fileLoadedSuccessfully) {
            tabTitle = QFileInfo(filePath).fileName();
        } else {
            // If loading fails, remove the tab and return without adding it
            openEditorTabs.removeOne(editorTab);
            editorTab->deleteLater(); // Schedule for deletion
            QMessageBox::warning(this, tr("File Open Error"),
                                 tr("Could not open file: %1").arg(filePath));
            return;
        }
    }

    int index = ui->tabWidgetEditor->addTab(editorTab, tabTitle);
    ui->tabWidgetEditor->setCurrentIndex(index); // Make the new tab active

    // For a new, empty tab, mark it as modified to trigger a save prompt on close
    if (filePath.isEmpty()) {
        editorTab->setModified(true); // New empty tab is considered modified
        updateTabTitle(true); // Manually call to set initial (modified) title
    }
}

bool MainWindow::saveCurrentFile()
{
    EditorTabWidget *editorTab = currentEditorTab();
    if (!editorTab) return true; // No active tab, nothing to save

    if (!editorTab->isModified()) return true; // Not modified, no need to save

    QMessageBox::StandardButton ret;
    QString fileName = QFileInfo(editorTab->filePath()).fileName();
    if (fileName.isEmpty()) {
        fileName = tr("Untitled"); // For new, unsaved files
    }

    ret = QMessageBox::warning(this, tr("Save Changes"),
                               tr("The document '%1' has been modified.\nDo you want to save your changes?").arg(fileName),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        if (editorTab->filePath().isEmpty()) {
            return on_actionSaveFileAs_triggered(); // Call Save As for new files
        } else {
            return editorTab->saveFile(editorTab->filePath()); // Save to existing path
        }
    } else if (ret == QMessageBox::Cancel) {
        return false; // User cancelled save operation
    }
    return true; // Discard was chosen, or successfully saved
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Iterate through all open tabs and check for unsaved changes
    // Iterate backwards or use a copy if elements might be removed
    // (though saveCurrentFile only affects current, and removeOne in on_tabWidgetEditor_tabCloseRequested handles that)
    for (int i = 0; i < ui->tabWidgetEditor->count(); ++i) {
        EditorTabWidget *editorTab = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(i));
        if (editorTab && editorTab->isModified()) {
            ui->tabWidgetEditor->setCurrentIndex(i); // Make the tab current to show the warning clearly
            if (!saveCurrentFile()) { // This will handle the save/discard/cancel dialog
                event->ignore(); // User cancelled a save, prevent closing the app
                return;
            }
        }
    }
    event->accept(); // All tabs checked/saved/discarded, allow closing the app
}

void MainWindow::on_actionNewFile_triggered()
{
    createNewTab();
}

void MainWindow::on_actionOpenFile_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), QString(), tr("Text Files (*.txt *.vb *.cpp *.h *.kayte *.kyt *.pas *.pp *.dpr);;All Files (*.*)"));
    if (filePath.isEmpty()) return;

    // Check if the file is already open in another tab
    for (EditorTabWidget *tab : openEditorTabs) {
        if (!tab->filePath().isEmpty() && QFileInfo(tab->filePath()) == QFileInfo(filePath)) {
            ui->tabWidgetEditor->setCurrentWidget(tab); // Switch to existing tab
            return;
        }
    }

    createNewTab(filePath); // Open the file in a new tab
}

void MainWindow::on_actionSaveFile_triggered()
{
    EditorTabWidget *editorTab = currentEditorTab();
    if (!editorTab) return; // No active tab

    if (editorTab->filePath().isEmpty()) {
        // If no path, treat as Save As
        on_actionSaveFileAs_triggered();
    } else {
        editorTab->saveFile(editorTab->filePath());
        // `editorTab->saveFile` should emit modificationChanged(false) which updates the title.
        // No explicit updateTabTitle(false) needed here.
    }
}

bool MainWindow::on_actionSaveFileAs_triggered()
{
    EditorTabWidget *editorTab = currentEditorTab();
    if (!editorTab) return false; // No active tab

    QString initialPath = editorTab->filePath().isEmpty() ? QDir::homePath() : editorTab->filePath();
    QString newFilePath = QFileDialog::getSaveFileName(this, tr("Save File As"), initialPath, tr("Text Files (*.txt *.vb *.cpp *.h *.kayte *.kyt *.pas *.pp *.dpr);;All Files (*.*)"));
    if (newFilePath.isEmpty()) return false;

    bool saved = editorTab->saveFile(newFilePath);
    if (saved) {
        // editorTab->saveFile will emit modificationChanged(false) and titleChanged
        // So updateTabTitleOnRename is handled by that connection.
    }
    return saved;
}

void MainWindow::on_actionCloseTab_triggered()
{
    int currentIndex = ui->tabWidgetEditor->currentIndex();
    if (currentIndex != -1) { // -1 means no tab is open
        // Reuse the on_tabWidgetEditor_tabCloseRequested logic
        on_tabWidgetEditor_tabCloseRequested(currentIndex);
    }
}

void MainWindow::on_tabWidgetEditor_tabCloseRequested(int index)
{
    if (index < 0 || index >= ui->tabWidgetEditor->count()) return;

    EditorTabWidget *tabToClose = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(index));
    if (!tabToClose) return;

    ui->tabWidgetEditor->setCurrentIndex(index); // Make it current to show warning dialog contextually

    if (tabToClose->isModified()) {
        if (!saveCurrentFile()) { // saveCurrentFile will handle dialogs for the current (this) tab
            return; // User cancelled saving or closing this specific tab
        }
    }

    // If we reached here, it's safe to close the tab (saved, discarded, or not modified)
    openEditorTabs.removeOne(tabToClose); // Remove from our tracking list
    ui->tabWidgetEditor->removeTab(index); // Remove from QTabWidget. This implicitly reparents to 0 or deletes if no other parent.
    tabToClose->deleteLater(); // Ensure proper deletion, good practice even if parented.

    if (ui->tabWidgetEditor->count() == 0) {
        // If the last tab is closed, create a new empty one to keep the UI active
        createNewTab();
    }
}

void MainWindow::updateTabTitle(bool modified)
{
    EditorTabWidget *editorTab = qobject_cast<EditorTabWidget*>(sender()); // Get the sender (which EditorTabWidget emitted the signal)
    if (!editorTab) return;

    int index = ui->tabWidgetEditor->indexOf(editorTab);
    if (index != -1) {
        QString title = QFileInfo(editorTab->filePath()).fileName();
        if (title.isEmpty()) {
            title = tr("Untitled");
        }
        if (modified) {
            title += "*"; // Add asterisk for unsaved changes
        }
        ui->tabWidgetEditor->setTabText(index, title);
    }
}

void MainWindow::updateTabTitleOnRename(const QString &newTitle)
{
    EditorTabWidget *editorTab = qobject_cast<EditorTabWidget*>(sender());
    if (!editorTab) return;

    int index = ui->tabWidgetEditor->indexOf(editorTab);
    if (index != -1) {
        QString title = newTitle; // New file name
        if (editorTab->isModified()) {
            title += "*"; // Still add asterisk if it's currently modified
        }
        ui->tabWidgetEditor->setTabText(index, title);
    }
}

void MainWindow::buildProject()
{
    if (currentEditorTab()) {
        QMessageBox::information(this, tr("Build"), tr("Build project action triggered for current file: %1").arg(currentEditorTab()->filePath()));
    } else {
        QMessageBox::information(this, tr("Build"), tr("No active editor tab. Build project action triggered."));
    }
}

void MainWindow::runProject()
{
    if (currentEditorTab()) {
        QMessageBox::information(this, tr("Run"), tr("Run project action triggered for current file: %1").arg(currentEditorTab()->filePath()));
    } else {
        QMessageBox::information(this, tr("Run"), tr("No active editor tab. Run project action triggered."));
    }
}

void MainWindow::cleanProject()
{
    QMessageBox::information(this, tr("Clean"), tr("Clean project action triggered."));
}

void MainWindow::debugProject()
{
    if (currentEditorTab()) {
        QMessageBox::information(this, tr("Debug"), tr("Debug project action triggered for current file: %1").arg(currentEditorTab()->filePath()));
    } else {
        QMessageBox::information(this, tr("Debug"), tr("No active editor tab. Debug project action triggered."));
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(tr("About Kayte IDE"));
    aboutBox.setIconPixmap(QIcon::fromTheme("help-about").pixmap(64, 64)); // Use fromTheme for better icon integration
    aboutBox.setTextFormat(Qt::RichText);

    QString aboutText = tr("about_dialog_text_html"); // Get translated text from your JSON
    aboutBox.setText(aboutText);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void MainWindow::showModeSelectionDialog()
{
    ChoiceMode dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        ChoiceMode::DevelopmentMode chosenMode = dialog.getSelectedMode();
        currentDevelopmentMode = chosenMode; // Store the chosen mode
        activateMode(chosenMode); // Pass the chosen mode enum directly
    } else {
        QMessageBox::information(this, tr("Mode Selection"), tr("No mode selected. Defaulting to Text Editor Mode."));
        currentDevelopmentMode = ChoiceMode::TextEditor; // Default if canceled
        activateMode(ChoiceMode::TextEditor);
    }
}

// THIS IS THE CORRECTED DEFINITION
void MainWindow::activateMode(ChoiceMode::DevelopmentMode mode)
{
    qDebug() << "Activating mode:" << mode; // Debugging output

    DownloadProgressDialog *downloadDialog = new DownloadProgressDialog(this);
    downloadDialog->setAttribute(Qt::WA_DeleteOnClose); // Ensure dialog is deleted after closing

    // Connect to the overall completion or abortion of the download process
    connect(downloadDialog, &DownloadProgressDialog::processCompleted,
            this, &MainWindow::handleDownloadDialogFinished);
    connect(downloadDialog, &DownloadProgressDialog::processAborted,
            this, &MainWindow::handleDownloadDialogFinished); // Also handle abortion as finishing

    // Determine which repositories to download based on the chosen mode
    QStringList reposToUse;
    if (mode == ChoiceMode::TextEditor) {
        QMessageBox::information(this, tr("Mode Activated"), tr("Text Editor Mode activated!"));
        ui->statusbar->showMessage(tr("Mode: Text Editor"), 3000);
        reposToUse = editorModeRepos;
    } else if (mode == ChoiceMode::RAD) {
        QMessageBox::information(this, tr("Mode Activated"), tr("RAD Mode activated!"));
        ui->statusbar->showMessage(tr("Mode: RAD"), 3000);
        reposToUse = radModeRepos;
    } else {
        qWarning() << "Unknown development mode selected.";
        ui->statusbar->showMessage(tr("Mode: Unknown"), 3000);
    }

    // Start the download process (which includes prerequisite checks internally)
    // Pass the list of repos and the destination directory
    downloadDialog->startProcess(reposToUse, defaultDownloadPath);
    downloadDialog->show(); // Show the dialog
}

void MainWindow::handleDownloadDialogFinished()
{
    // This slot is now called when DownloadProgressDialog::processCompleted or processAborted is emitted
    QMessageBox::information(this, tr("Initialization Complete"), tr("IDE is ready!"));
    ui->statusbar->showMessage(tr("IDE Ready."), 3000);

    // Based on the 'currentDevelopmentMode' established earlier, you can adjust UI.
    if (currentDevelopmentMode == ChoiceMode::RAD) {
        // Example: Enable specific RAD-related widgets/menus
        // ui->actionBuild, ui->actionRun, etc. might become more prominent or enabled
        // ui->someRadPanel->setVisible(true);
        // ui->someTextEditorPanel->setVisible(false);
    } else { // TextEditor mode
        // ui->someTextEditorPanel->setVisible(true);
        // ui->someRadPanel->setVisible(false);
    }
}

// In mainwindow.cpp
void MainWindow::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    // This slot is typically connected to QPlainTextEdit::blockCountChanged
    // for updating a LineNumberArea.
    // If lineNumberArea is now managed within each EditorTabWidget,
    // this specific slot in MainWindow might not be directly used
    // or would delegate to the current tab.
    // For now, an empty definition will resolve the linker error.
}

// In mainwindow.cpp
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event); // Always call the base class implementation!
    // If you had a standalone lineNumberArea here previously,
    // it would be removed now. The QTabWidget handles its layout.
    // No specific custom resize logic needed unless you add non-tab-related elements.
}

void MainWindow::setupDownloadRepos()
{
    radModeRepos << "[https://github.com/ringsce/kayte-lang.git;kayte_lang;main](https://github.com/ringsce/kayte-lang.git;kayte_lang;main)"
                 << "[https://github.com/KayteIDE/rad-templates.git;rad_templates](https://github.com/KayteIDE/rad-templates.git;rad_templates)"
                 << "[https://github.com/KayteIDE/samples.git;rad_samples;dev](https://github.com/KayteIDE/samples.git;rad_samples;dev)"
                 << "[https://github.com/KayteIDE/documentation.git;docs](https://github.com/KayteIDE/documentation.git;docs)"
                 << "[https://github.com/KayteIDE/tutorials.git;tutorials](https://github.com/KayteIDE/tutorials.git;tutorials)";

    editorModeRepos << "[https://github.com/KayteIDE/editor-addons.git;editor_addons](https://github.com/KayteIDE/editor-addons.git;editor_addons)";
}