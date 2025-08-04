#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <QObject>
#include <QPointer> // QPointer for safe handling of QObject-derived pointers

// Forward declarations to avoid including heavy headers in the .h file
class QPlainTextEdit; // Assuming your editor uses QPlainTextEdit
class QTextEdit;      // Or if it uses QTextEdit

class KeyboardShortcutsManager : public QObject
{
    Q_OBJECT

public:
    explicit KeyboardShortcutsManager(QObject *parent = nullptr);

    // Call this method whenever the active editor changes
    void setTargetEditor(QPlainTextEdit* editor);
    // OR if you use QTextEdit:
    // void setTargetEditor(QTextEdit* editor);

public slots:
    // These slots will be connected to your QActions (e.g., from menus)
    void triggerSelectAll();
    void triggerCopy();
    void triggerPaste();
    void triggerCut();

private:
    // Use QPointer to safely hold a pointer to the target editor.
    // If the editor is deleted, m_targetEditor will automatically become nullptr.
    QPointer<QPlainTextEdit> m_targetEditor; // Change to QTextEdit* if that's what you use
};

#endif // KEYBOARD_H