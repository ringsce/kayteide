#include "choicemode.h"
#include "ui_choicemode.h" // Generated from your .ui file
#include <QPushButton> // To access the individual buttons in QDialogButtonBox

ChoiceMode::ChoiceMode(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChoiceMode)
{
    ui->setupUi(this);

    setWindowTitle("Choose Mode");

    // Default selection
    ui->radioButtonTextMode->setChecked(true);
    selectedMode = TextEditor; // Set initial mode

    // Get the "Ok" button from the button box and change its text to "Next"
    QPushButton *nextButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (nextButton) {
        nextButton->setText("Next");
    }

    // Connect the accepted signal of the button box
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ChoiceMode::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject); // Connect cancel as well
}

ChoiceMode::~ChoiceMode()
{
    delete ui;
}

// Slot called when the "Next" (Ok) button is clicked
void ChoiceMode::on_buttonBox_accepted()
{
    if (ui->radioButtonTextMode->isChecked()) {
        selectedMode = TextEditor;
    } else if (ui->radioButtonRADMode->isChecked()) {
        selectedMode = RAD;
    }
    // Call accept() on the dialog to close it with accepted status
    accept();
}

ChoiceMode::DevelopmentMode ChoiceMode::getSelectedMode() const
{
    return selectedMode;
}