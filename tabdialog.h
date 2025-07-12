// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <QDialog>
#include <QFileInfo> // For QFileInfo

// Forward declarations to avoid circular includes for tab classes if they don't need TabDialog details
class QTabWidget;
class QDialogButtonBox;
class GeneralTab;
class PermissionsTab;
class ApplicationsTab;

class TabDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TabDialog(const QString &fileName, QWidget *parent = nullptr);

private:
    QTabWidget *tabWidget;
    QDialogButtonBox *buttonBox;
};

#endif // TABDIALOG_H