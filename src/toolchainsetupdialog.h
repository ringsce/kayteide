#ifndef TOOLCHAINSETUPDIALOG_H
#define TOOLCHAINSETUPDIALOG_H

#include <QDialog>

class QLabel;
class QPlainTextEdit;
class QProcess;
class QProgressBar;
class QPushButton;

// Runs the bundled toolchain installer (Contents/tools/scripts/requirements.sh)
// and shows its progress: Rosetta 2, the Xcode Command Line Tools, Free Pascal,
// Lazarus, the Kayte SDK and QEMU are installed into KayteIDE.app/Contents/tools.
class ToolchainSetupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ToolchainSetupDialog(QWidget *parent = nullptr);
    ~ToolchainSetupDialog() override;

    // The installer bundled with this app, or empty (e.g. not macOS).
    static QString installerScript();
    // True once the installer has completed for this app.
    static bool isInstalled();

    void start();

signals:
    // ok: the installer finished successfully. cancelled: stopped by the user.
    void finished(bool ok, bool cancelled);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void onOutput();
    void onFinished(int exitCode, bool crashed);
    bool confirmCancel();
    void stopInstaller();

    QLabel         *m_step     { nullptr };
    QProgressBar   *m_progress { nullptr };
    QPlainTextEdit *m_log      { nullptr };
    QPushButton    *m_details  { nullptr };
    QPushButton    *m_cancel   { nullptr };
    QPushButton    *m_retry    { nullptr };
    QPushButton    *m_close    { nullptr };
    QProcess       *m_process  { nullptr };
    QByteArray      m_pending;
    QString         m_lastError;
    bool            m_cancelled { false };
};

#endif // TOOLCHAINSETUPDIALOG_H
