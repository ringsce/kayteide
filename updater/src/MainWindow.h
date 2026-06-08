#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QSystemTrayIcon>
#include <QMenu>

#include "UpdateChecker.h"
#include "BuildManager.h"
#include "LogWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onUpdateAvailable(const CommitInfo &commit);
    void onAlreadyUpToDate();
    void onCheckFailed(const QString &error);
    void onCheckStarted();

    void onBuildStepChanged(BuildStep step, const QString &desc);
    void onBuildSucceeded();
    void onBuildFailed(const QString &reason);

    void onCheckNowClicked();
    void onBuildClicked();
    void onAbortClicked();
    void onSettingsChanged();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupUi();
    void setupTray();
    void applyStyleSheet();
    void setStatus(const QString &text, const QString &color = "#c9d1d9");
    void setControlsEnabled(bool enabled);

    // Core logic
    UpdateChecker *m_checker  = nullptr;
    BuildManager  *m_builder  = nullptr;

    // UI – Status bar area
    QLabel      *m_statusLabel     = nullptr;
    QLabel      *m_commitLabel     = nullptr;
    QLabel      *m_lastCheckLabel  = nullptr;
    QProgressBar *m_progressBar    = nullptr;

    // UI – Buttons
    QPushButton *m_checkBtn  = nullptr;
    QPushButton *m_buildBtn  = nullptr;
    QPushButton *m_abortBtn  = nullptr;

    // UI – Settings
    QLineEdit *m_sourceDirEdit  = nullptr;
    QLineEdit *m_buildDirEdit   = nullptr;
    QLineEdit *m_installDirEdit = nullptr;
    QSpinBox  *m_jobsSpin       = nullptr;
    QSpinBox  *m_intervalSpin   = nullptr;
    QCheckBox *m_autoCheckBox   = nullptr;
    QCheckBox *m_installChk     = nullptr;

    // UI – Log
    LogWidget *m_log = nullptr;

    // Tray
    QSystemTrayIcon *m_tray     = nullptr;
    QMenu           *m_trayMenu = nullptr;

    bool m_updatePending = false;
};
