#include "mainwindow.h"
#include "ui_mainwindow.h" // If you use Qt Designer

// Qt Includes specific to mainwindow.cpp implementation details
#include <QFileDialog>      // Used for QFileDialog::getExistingDirectory
#include <QStandardPaths>   // Used for QStandardPaths::homeLocation() or QDir::homePath()
#include <QDir>             // Used for QDir::exists(), QDir::toNativeSeparators(), etc.
#include <QDebug>           // Used for qDebug() and qWarning()

// Other Qt includes from your original snippet that you might use in this .cpp file
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QIcon>
#include <QFileInfo>
#include <QPixmap>
#include <QTextDocument>
#include <QFont>
#include <QCloseEvent>

// Your custom widget and dialog includes
#include "editortabwidget.h"
#include "choicemode.h"
#include "downloadprogressdialog.h"

#include <QXmlStreamWriter>
#include "keyboard.h" // Ensure this is included here as well




MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_keyboardShortcutsManager(new KeyboardShortcutsManager(this)) // CORRECT: Only initialize here
{
    ui->setupUi(this);  // Initialize UI from mainwindow.ui
    setWindowTitle(tr("Kayte IDE"));

    qDebug() << "DEBUG: MainWindow constructor - ui pointer:" << ui;
    if (ui) {
        qDebug() << "DEBUG: MainWindow constructor - ui->tabWidgetEditor pointer:" << ui->tabWidgetEditor;
    } else {
        qDebug() << "DEBUG: MainWindow constructor - ui is nullptr!";
    }

    // --- Set Central Widget ---
    setCentralWidget(ui->tabWidgetEditor);

    // --- Editor Tabs Setup ---
    ui->tabWidgetEditor->setTabsClosable(true);
    ui->tabWidgetEditor->setMovable(true);

    connect(ui->tabWidgetEditor, &QTabWidget::tabCloseRequested,
            this, &MainWindow::on_tabWidgetEditor_tabCloseRequested);

    // Create an initial empty tab
    createNewTab();

    // --- File Paths / Directory Setup ---
    defaultDownloadPath = QDir::homePath() + QDir::separator() + "KayteIDE_Resources";
    QDir().mkpath(defaultDownloadPath); // Ensure the directory exists

    // --- Download Repositories Setup ---
    setupDownloadRepos();

    // --- Menu & Toolbar Icons ---
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

    // --- Menu Action Connections ---
    connect(ui->actionNewFile, &QAction::triggered, this, &MainWindow::on_actionNewFile_triggered);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::handleOpenFileTriggered);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::handleSaveFileTriggered);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::handleSaveFileAsTriggered);
    connect(ui->actionCloseTab, &QAction::triggered, this, &MainWindow::on_actionCloseTab_triggered);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui->actionBuild, &QAction::triggered, this, &MainWindow::buildProject);
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::runProject);
    connect(ui->actionClean, &QAction::triggered, this, &MainWindow::cleanProject);
    connect(ui->actionDebug, &QAction::triggered, this, &MainWindow::debugProject);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(ui->actionSaveProjectAs, &QAction::triggered, this, &MainWindow::saveProjectAs);

    // This connection now works as requested
    connect(ui->actionNewProject, &QAction::triggered, this, &MainWindow::on_actionNewProject_triggered);

    // --- Mode Selection Dialog ---
    QMetaObject::invokeMethod(this, "showModeSelectionDialog", Qt::QueuedConnection);

    // Connect standard edit actions from your menu/toolbar to the manager's slots.
    connect(ui->actionCut,       &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerCut);
    connect(ui->actionCopy,      &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerCopy);
    connect(ui->actionPaste,     &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerPaste);
    connect(ui->actionSelectAll, &QAction::triggered, m_keyboardShortcutsManager, &KeyboardShortcutsManager::triggerSelectAll);

    // Connect a slot to handle changes in the active editor tab.
    connect(ui->tabWidgetEditor, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidgetEditor_currentChanged);
}

void MainWindow::on_actionNewProject_triggered()
{
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // TODO: Add your logic here to get the project options from the dialog
        // and create the new project.
        // For example:
        // QString projectName = dialog.getProjectName();
        // QString projectPath = dialog.getProjectPath();
        // QString projectType = dialog.getProjectType();
        // ... then create project files based on the type
    }
}
// Fix this function name to match the declaration in mainwindow.h
void MainWindow::handleOpenFileTriggered() // <--- CHANGE THIS LINE
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                    tr("All Files (*);;Text Files (*.txt);;Source Files (*.cpp *.h *.cxx *.hpp);;Visual Basic (*.vb);;Kayte Files (*.kayte *.kyt);;Pascal Files (*.pas *.pp *.dpr);;Delphi Forms (*.dfm)"));
    if (filePath.isEmpty()) {
        return; // User cancelled
    }

    // Check if the file is already open
    for (int i = 0; i < ui->tabWidgetEditor->count(); ++i) {
        EditorTabWidget *existingTab = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(i));
        if (existingTab && existingTab->filePath() == filePath) {
            ui->tabWidgetEditor->setCurrentIndex(i);
            return; // File is already open, just switch to its tab
        }
    }

    // Create a new tab for the file:
    EditorTabWidget *newTab = new EditorTabWidget(filePath, ui->tabWidgetEditor);

    if (newTab->loadFile(filePath)) { // Ensure loadFile handles errors and returns bool
        int tabIndex = ui->tabWidgetEditor->addTab(newTab, QFileInfo(filePath).fileName());
        ui->tabWidgetEditor->setCurrentIndex(tabIndex);

        // Connect modificationChanged and titleChanged signals from the new tab
        connect(newTab, &EditorTabWidget::modificationChanged, this, &MainWindow::updateTabTitle);
        connect(newTab, &EditorTabWidget::titleChanged, this, &MainWindow::updateTabTitleOnRename); // Use updateTabTitleOnRename

        // Connect for closing tab
        connect(newTab, &EditorTabWidget::destroyed, this, &MainWindow::onTabClosed); // Ensure this signal/slot exists
    } else {
        newTab->deleteLater(); // Clean up if file failed to load
    }
}

// In src/mainwindow.cpp, inside MainWindow::on_tabWidgetEditor_currentChanged(int index)

void MainWindow::on_tabWidgetEditor_currentChanged(int index)
{
    EditorTabWidget *currentTab = qobject_cast<EditorTabWidget*>(ui->tabWidgetEditor->widget(index));
    if (currentTab) {
        // Ensure EditorTabWidget::getPlainTextEdit() returns a QPlainTextEdit*
        m_keyboardShortcutsManager->setTargetEditor(currentTab->getPlainTextEdit());
    } else {
        m_keyboardShortcutsManager->setTargetEditor(nullptr); // No editor tab is active
    }
}

// And in your MainWindow constructor, if you have a default tab created:
// Example (adjust based on your actual initial tab creation logic):
// if (ui->tabWidgetEditor->count() > 0) {
//     on_tabWidgetEditor_currentChanged(ui->tabWidgetEditor->currentIndex());
// }

// --- IMPORTANT NOTE FOR EDITOR TAB WIDGET ---
// In your EditorTabWidget class (editortabwidget.h and editortabwidget.cpp),
// you will need a public getter method to return a pointer to the
// QPlainTextEdit (or QTextEdit) that it contains.
//
// Example in editortabwidget.h:
// class EditorTabWidget : public QWidget {
//     Q_OBJECT
// public:
//     // ... constructor etc. ...
//     QPlainTextEdit* getPlainTextEdit() const { return m_editor; } // Assuming m_editor is your QPlainTextEdit*
//     // OR:
//     // QTextEdit* getTextEdit() const { return m_editor; }
// private:
//     QPlainTextEdit* m_editor; // Your actual text editor widget
// };

void MainWindow::setupFileBrowser()
{
    // --- 1. Create the QFileSystemModel ---
    fileSystemModel = new QFileSystemModel(this);
    fileSystemModel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries); // Exclude . and ..
    fileSystemModel->setRootPath(QDir::homePath()); // Start the model at user's home dir

    // --- 2. Create the QListView (or QTreeView) ---
    // Using QTreeView is generally better for file Browse
    QTreeView *fileTreeView = new QTreeView(this); // Let's use QTreeView instead of QListView
    fileTreeView->setModel(fileSystemModel);

    // Optional: Hide columns you don't need (QTreeView has multiple columns by default)
    fileTreeView->hideColumn(1); // Hide Size column
    fileTreeView->hideColumn(2); // Hide Type column
    fileTreeView->hideColumn(3); // Hide Date Modified column
    // Or, keep them for a detailed list view!

    // In your setupFileBrowser() function (in mainwindow.cpp)
    // Find the connect call related to pathLineEdit:
    // connect(pathLineEdit, &QLineEdit::returnPressed, this, &MainWindow::on_pathLineEdit_returnPressed);
    // Change it to:
    // --- 3. Path Line Edit ---
    pathLineEdit = new QLineEdit(this);
    connect(pathLineEdit, &QLineEdit::returnPressed, this, &MainWindow::handlePathLineEditReturnPressed); // <--- ENSURE THIS IS THE NAME

    // Optional: Browse button for a folder dialog
    browseButton = new QPushButton("Browse...", this);
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Open Directory",
                                                        pathLineEdit->text(),
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            setCurrentPath(dir);
        }
    });

    // --- 4. Layout for your window ---
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("Path:", this));
    pathLayout->addWidget(pathLineEdit);
    pathLayout->addWidget(browseButton);

    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(fileTreeView); // Add the tree view

    setCentralWidget(centralWidget);

    // Store the tree view in the fileListView pointer for consistency
    // If you explicitly wanted a QListView, you'd use that here and
    // it wouldn't show columns other than name.
    fileListView = fileTreeView; // Point to the QTreeView
}

void MainWindow::setCurrentPath(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        pathLineEdit->setText(QDir::toNativeSeparators(path));
        // Set the root path for the model to the current directory
        fileListView->setRootIndex(fileSystemModel->setRootPath(path));
    } else {
        qWarning() << "Path does not exist:" << path;
    }
}

void MainWindow::handlePathLineEditReturnPressed() // <-- This is the correct definition name
{
    // Your existing code for handling return pressed on pathLineEdit goes here.
    // For example, it might look something like this:
    QString newPath = pathLineEdit->text();
    QDir dir(newPath);

    if (dir.exists()) {
        setCurrentPath(newPath);
    } else {
        QMessageBox::warning(this, tr("Path Not Found"),
                             tr("The path '%1' does not exist.").arg(newPath));
        // Optionally revert the text to the current valid path if the new one is invalid
        pathLineEdit->setText(fileSystemModel->rootPath());
    }
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

// Implement the new slot
void MainWindow::onTabClosed(QObject* obj)
{
    // This slot is called when an EditorTabWidget object is destroyed.
    // The QTabWidget typically handles removing the tab and deleting the widget.
    // You might use this for additional cleanup, like removing it from your openEditorTabs vector.
    // Example:
    EditorTabWidget* destroyedTab = qobject_cast<EditorTabWidget*>(obj);
    if (destroyedTab) {
        qDebug() << "EditorTabWidget destroyed:" << destroyedTab->filePath();
        // If you're manually managing openEditorTabs, you might remove it here:
        // openEditorTabs.removeOne(destroyedTab);
    }
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

    // <--- CORRECTED THIS LINE: Pass filePath as the first argument and ui->tabWidgetEditor as the parent
    EditorTabWidget *editorTab = new EditorTabWidget(filePath, ui->tabWidgetEditor);
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
        updateTabTitle(true); // Manually call to set initial (modified) title, this will update based on current tab index
    }
    // Set keyboard manager target for the newly created tab
    m_keyboardShortcutsManager->setTargetEditor(editorTab->getPlainTextEdit());
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
            return handleSaveFileAsTriggered(); // ***FIXED: CALL NEW SLOT NAME HERE***
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


// Change the function definition line from:
// void MainWindow::on_actionSaveFile_triggered()
// TO:
void MainWindow::handleSaveFileTriggered() // <--- FIX IS HERE: Renamed function definition
{
    EditorTabWidget *editorTab = currentEditorTab();
    if (!editorTab) return; // No active tab

    if (editorTab->filePath().isEmpty()) {
        // If no path, treat as Save As
        handleSaveFileAsTriggered(); // <--- FIX IS HERE: Call the renamed 'Save As' slot
    } else {
        editorTab->saveFile(editorTab->filePath());
        // `editorTab->saveFile` should emit modificationChanged(false) which updates the title.
        // No explicit updateTabTitle(false) needed here.
    }
}

void MainWindow::handleTabModificationChanged(bool modified)
{
    // This slot is called when a tab's modification status changes.
    // You'll likely want to update the tab's title to show an asterisk (*) for modified files.
    // Example (you might have similar logic in updateTabTitle):
    EditorTabWidget *editorTab = qobject_cast<EditorTabWidget*>(sender());
    if (editorTab) {
        int index = ui->tabWidgetEditor->indexOf(editorTab);
        if (index != -1) {
            QString title = QFileInfo(editorTab->filePath()).fileName();
            if (modified) {
                title += "*";
            }
            ui->tabWidgetEditor->setTabText(index, title);
        }
    }
    // You might also want to enable/disable the Save action here
    ui->actionSave->setEnabled(modified);
    ui->actionSave_As->setEnabled(modified); // Save As usually enabled if anything is open
}

void MainWindow::handleTabTitleChanged(const QString &newTitle)
{
    // This slot is called when a tab's internal title (e.g., file path) changes.
    // You'll update the tab's displayed text.
    EditorTabWidget *editorTab = qobject_cast<EditorTabWidget*>(sender());
    if (editorTab) {
        int index = ui->tabWidgetEditor->indexOf(editorTab);
        if (index != -1) {
            ui->tabWidgetEditor->setTabText(index, newTitle);
        }
    }
}

void MainWindow::saveProjectAs()
{
    // 1. Get the desired save file path from the user
    QString initialPath = m_currentProjectFilePath.isEmpty() ?
                          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) :
                          QFileInfo(m_currentProjectFilePath).absolutePath();

    QString saveFilePath = QFileDialog::getSaveFileName(this,
                                                        tr("Save Project As"),
                                                        initialPath,
                                                        tr("Kayte IDE Project Files (*.xprj);;All Files (*.*)"));

    if (saveFilePath.isEmpty()) {
        return; // User cancelled the dialog
    }

    // Ensure the file has the .xprj extension
    if (!saveFilePath.endsWith(".xprj", Qt::CaseInsensitive)) {
        saveFilePath += ".xprj";
    }

    // Update current project path and name based on the chosen file
    QFileInfo fileInfo(saveFilePath);
    m_currentProjectFilePath = saveFilePath;
    m_currentProjectName = fileInfo.baseName(); // Name of the file without extension

    // 2. Open the file for writing
    QFile file(saveFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error Saving Project"),
                               tr("Cannot write file: %1\n%2").arg(saveFilePath, file.errorString()));
        return;
    }

    // 3. Set up the XML writer
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.setAutoFormattingIndent(4);
    xmlWriter.writeStartDocument(); // Writes the XML declaration (e.g., <?xml version="1.0"?>)

    // 4. Write the root element
    xmlWriter.writeStartElement("KayteIDEProject");

    // 5. Write basic project information
    xmlWriter.writeTextElement("ProjectName", m_currentProjectName);
    xmlWriter.writeTextElement("ProjectPath", fileInfo.absoluteDir().path()); // The directory where the .xprj is saved

    // Convert DevelopmentMode enum to string for saving
    QString modeString;
    switch (currentDevelopmentMode) {
        // Corrected lines:
    case ChoiceMode::TextEditor: // Use the correct enum name from choicemode.h
        modeString = "Editor";   // The string representation you want to save
        break;
    case ChoiceMode::RAD:        // Use the correct enum name from choicemode.h
        modeString = "RAD";      // The string representation you want to save
        break;
    default:
        modeString = "Unknown"; // Fallback for any unhandled modes
        break;
    }
    xmlWriter.writeTextElement("DevelopmentMode", modeString);
    // 6. Write Build Settings
    xmlWriter.writeStartElement("BuildSettings");
    xmlWriter.writeTextElement("BuildCommand", m_buildCommand);
    xmlWriter.writeTextElement("RunCommand", m_runCommand);
    xmlWriter.writeTextElement("CleanCommand", m_cleanCommand);
    xmlWriter.writeTextElement("DebugCommand", m_debugCommand);
    xmlWriter.writeEndElement(); // </BuildSettings>

    // 7. Write currently open files
    xmlWriter.writeStartElement("OpenFiles");
    for (EditorTabWidget *tab : openEditorTabs) {
        // Only save files that have a path (i.e., not a new, unsaved tab)
        if (!tab->filePath().isEmpty()) {
            xmlWriter.writeTextElement("File", tab->filePath());
        }
    }
    xmlWriter.writeEndElement(); // </OpenFiles>

    // 8. End the root element and the document
    xmlWriter.writeEndElement(); // </KayteIDEProject>
    xmlWriter.writeEndDocument();

    // 9. Close the file
    file.close();

    statusBar()->showMessage(tr("Project \"%1\" saved successfully.").arg(m_currentProjectName), 3000);
    qDebug() << "Project saved to:" << m_currentProjectFilePath;
}
// In mainwindow.cpp
void MainWindow::handleListViewDoubleClicked(const QModelIndex &index) // <-- FIX IS HERE: Renamed to match declaration
{
    // ... your existing code ...
    // This function should contain the logic you had for handling double-clicks
    // on the file list view. For example:
    if (!index.isValid()) {
        return;
    }

    QFileInfo fileInfo = fileSystemModel->fileInfo(index);
    if (fileInfo.isDir()) {
        setCurrentPath(fileInfo.absoluteFilePath());
    } else {
        // If it's a file, open it in a new editor tab
        createNewTab(fileInfo.absoluteFilePath());
    }
}


bool MainWindow::handleSaveFileAsTriggered() // <--- FIX IS HERE
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

// diownload repos
// In mainwindow.cpp
void MainWindow::setupDownloadRepos()
{
    // CORRECTED FORMAT: "GIT_URL;LOCAL_DIR_NAME;BRANCH_NAME"
    // (BRANCH_NAME is optional. If not provided, 'main' or default branch assumed by Git)

    radModeRepos << "https://github.com/ringsce/kayte-lang.git;kayte_lang;main"
                 << "https://github.com/ringsce/rad-templates.git;rad_templates"
                 << "https://github.com/ringsce/samples.git;rad_samples;dev"
                 << "https://github.com/ringsce/documentation.git;docs"
                 << "https://github.com/ringsce/tutorials.git;tutorials";

    editorModeRepos << "https://github.com/ringsce/editor_addons.git;editor_addons";
}


// NEW: Implementation for populateProjectList
void MainWindow::populateProjectList()
{
    // Clear any existing items
    ui->projectListWidget->clear();

    // Add some dummy project items for demonstration
    ui->projectListWidget->addItem(tr("My First RAD Project"));
    ui->projectListWidget->addItem(tr("Sample Game Engine"));
    ui->projectListWidget->addItem(tr("Business Application Prototype"));
    ui->projectListWidget->addItem(tr("Another Cool Project"));

    // You could later load these from a configuration file or scan a project directory
}

