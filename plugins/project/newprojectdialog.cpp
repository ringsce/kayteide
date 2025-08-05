#include "newprojectdialog.h"
#include "ui_newprojectdialog.h" // This is a generated header from the .ui file.

NewProjectDialog::NewProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProjectDialog)
{
    ui->setupUi(this);
}

NewProjectDialog::~NewProjectDialog()
{
    delete ui;
}