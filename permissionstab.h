// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PERMISSIONSTAB_H
#define PERMISSIONSTAB_H

#include <QWidget>
#include <QFileInfo> // For QFileInfo

class PermissionsTab : public QWidget
{
    Q_OBJECT

public:
    explicit PermissionsTab(const QFileInfo &fileInfo, QWidget *parent = nullptr);
};

#endif // PERMISSIONSTAB_H