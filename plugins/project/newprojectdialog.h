#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>

// Forward declaration to avoid including the full Ui header in this file.
namespace Ui {
    class NewProjectDialog;
}

class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);
    ~NewProjectDialog();

    // You can add public methods here to get the user's choices, e.g.:
    // QString getProjectName() const;
    // QString getProjectPath() const;
    // QString getProjectType() const;

private:
    Ui::NewProjectDialog *ui;
};

#endif // NEWPROJECTDIALOG_H