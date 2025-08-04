#include "keyboard.h"
#include <QPlainTextEdit> // Include the actual header for your editor widget
#include <QTextEdit>      // Include if you use QTextEdit instead of QPlainTextEdit

KeyboardShortcutsManager::KeyboardShortcutsManager(QObject *parent)
    : QObject(parent),
      m_targetEditor(nullptr) // Initialize the target editor pointer to null
{
}

// Set the editor that the manager's actions should apply to
void KeyboardShortcutsManager::setTargetEditor(QPlainTextEdit* editor)
{
    m_targetEditor = editor;
}
// OR if you use QTextEdit:
// void KeyboardShortcutsManager::setTargetEditor(QTextEdit* editor)
// {
//     m_targetEditor = editor;
// }


// Implement the shortcut trigger slots
void KeyboardShortcutsManager::triggerSelectAll()
{
    if (m_targetEditor) {
        m_targetEditor->selectAll(); // QPlainTextEdit has a built-in selectAll() slot
    }
}

void KeyboardShortcutsManager::triggerCopy()
{
    if (m_targetEditor) {
        m_targetEditor->copy(); // QPlainTextEdit has a built-in copy() slot
    }
}

void KeyboardShortcutsManager::triggerPaste()
{
    if (m_targetEditor) {
        m_targetEditor->paste(); // QPlainTextEdit has a built-in paste() slot
    }
}

void KeyboardShortcutsManager::triggerCut()
{
    if (m_targetEditor) {
        m_targetEditor->cut(); // QPlainTextEdit has a built-in cut() slot
    }
}
