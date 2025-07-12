// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef APPLICATIONSTAB_H
#define APPLICATIONSTAB_H

#include <QWidget>
#include <QFileInfo> // For QFileInfo

class ApplicationsTab : public QWidget
{
    Q_OBJECT

public:
    explicit ApplicationsTab(const QFileInfo &fileInfo, QWidget *parent = nullptr);
};

#endif // APPLICATIONSTAB_H