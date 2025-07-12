// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GENERALTAB_H
#define GENERALTAB_H

#include <QWidget>
#include <QFileInfo> // For QFileInfo

class GeneralTab : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralTab(const QFileInfo &fileInfo, QWidget *parent = nullptr);
};

#endif // GENERALTAB_H