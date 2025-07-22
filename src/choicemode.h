// choicemode.h
#ifndef CHOICEMODE_H
#define CHOICEMODE_H

#include <QDialog>
#include <QAbstractButton> // Needed for QDialogButtonBox::clicked

QT_BEGIN_NAMESPACE
namespace Ui { class ChoiceMode; }
QT_END_NAMESPACE

class ChoiceMode : public QDialog
{
    Q_OBJECT

public:
    explicit ChoiceMode(QWidget *parent = nullptr);
    ~ChoiceMode();

    // Enum to represent the chosen mode
    enum DevelopmentMode {
        TextEditor,
        RAD
    };

    DevelopmentMode getSelectedMode() const;

private slots:
    void on_buttonBox_accepted(); // Connected to the QDialogButtonBox's accepted() signal

private:
    Ui::ChoiceMode *ui;
    DevelopmentMode selectedMode; // Store the user's choice
};

#endif // CHOICEMODE_H