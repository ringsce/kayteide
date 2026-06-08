#pragma once

#include <QPlainTextEdit>

class LogWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);

public slots:
    void appendLog(const QString &line, bool isError = false);
    void appendSection(const QString &title);
    void clear();
};
