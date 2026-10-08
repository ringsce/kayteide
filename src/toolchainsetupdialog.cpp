#include "toolchainsetupdialog.h"
#include "buildconfig.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

#ifdef Q_OS_UNIX
#  include <signal.h>
#  include <unistd.h>
#endif

namespace {
// The installer's "==> …" steps, in order; used for the progress bar.
const char *const kSteps[] = {
    "Rosetta 2", "Xcode Command Line Tools", "Locating Lazarus", "Free Pascal Compiler",
    "Lazarus", "Kayte SDK", "QEMU", "Finishing",
};
constexpr int kStepCount = int(sizeof(kSteps) / sizeof(kSteps[0]));

QString appBundlePath()
{
    // KayteIDE.app/Contents/MacOS → KayteIDE.app
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../.."));
}
} // namespace

QString ToolchainSetupDialog::installerScript()
{
#ifdef Q_OS_MACOS
    const QString script = QDir::cleanPath(QCoreApplication::applicationDirPath()
                                           + QStringLiteral("/../tools/scripts/requirements.sh"));
    return QFileInfo(script).isFile() ? script : QString();
#else
    return {};
#endif
}

bool ToolchainSetupDialog::isInstalled()
{
    const QString tools = BuildConfigurations::toolsDir();
    return !tools.isEmpty() && QFileInfo::exists(tools + QStringLiteral("/VERSIONS.txt"));
}

ToolchainSetupDialog::ToolchainSetupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Install the KayteIDE Toolchain"));
    setMinimumWidth(620);

    auto *title = new QLabel(tr("<b>Installing the KayteIDE toolchain</b>"), this);
    auto *about = new QLabel(
        tr("Rosetta 2, the Xcode Command Line Tools, Free Pascal, Lazarus, the Kayte SDK "
           "and QEMU are installed into KayteIDE.app (about 2.6 GB). macOS may ask for "
           "your password (Rosetta 2) or to install the Command Line Tools. You can keep "
           "working while this runs."), this);
    about->setWordWrap(true);

    m_step = new QLabel(tr("Starting…"), this);
    m_progress = new QProgressBar(this);
    m_progress->setRange(0, kStepCount);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_log->setMaximumBlockCount(5000);
    m_log->setMinimumHeight(220);
    m_log->hide();

    m_details = new QPushButton(tr("Show Details"), this);
    m_details->setCheckable(true);
    connect(m_details, &QPushButton::toggled, this, [this](bool on) {
        m_log->setVisible(on);
        m_details->setText(on ? tr("Hide Details") : tr("Show Details"));
        adjustSize();
    });

    m_cancel = new QPushButton(tr("Cancel"), this);
    m_retry  = new QPushButton(tr("Retry"), this);
    m_close  = new QPushButton(tr("Close"), this);
    m_retry->hide();
    m_close->hide();
    connect(m_cancel, &QPushButton::clicked, this, [this] {
        if (confirmCancel()) stopInstaller();
    });
    connect(m_retry, &QPushButton::clicked, this, &ToolchainSetupDialog::start);
    connect(m_close, &QPushButton::clicked, this, &QDialog::accept);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_details);
    buttons->addStretch();
    buttons->addWidget(m_cancel);
    buttons->addWidget(m_retry);
    buttons->addWidget(m_close);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(about);
    layout->addSpacing(6);
    layout->addWidget(m_step);
    layout->addWidget(m_progress);
    layout->addWidget(m_log, 1);
    layout->addLayout(buttons);
}

ToolchainSetupDialog::~ToolchainSetupDialog()
{
    stopInstaller();
}

void ToolchainSetupDialog::start()
{
    const QString script = installerScript();
    if (script.isEmpty()) {
        onFinished(-1, true);
        return;
    }

    m_cancelled = false;
    m_lastError.clear();
    m_pending.clear();
    m_progress->setValue(0);
    m_step->setText(tr("Starting…"));
    m_cancel->show();
    m_cancel->setEnabled(true);
    m_retry->hide();
    m_close->hide();

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PATH"), QStringLiteral("/usr/local/bin:/opt/homebrew/bin:")
                                           + env.value(QStringLiteral("PATH")));
    m_process->setProcessEnvironment(env);
#ifdef Q_OS_UNIX
    // Own process group, so Cancel stops the installer and its downloads.
    m_process->setChildProcessModifier([] { ::setpgid(0, 0); });
#endif
    connect(m_process, &QProcess::readyReadStandardOutput, this, &ToolchainSetupDialog::onOutput);
    connect(m_process, &QProcess::finished, this,
            [this](int code, QProcess::ExitStatus status) { onFinished(code, status == QProcess::CrashExit); });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) onFinished(-1, true);
    });

    m_log->appendPlainText(QStringLiteral("$ %1 --rosetta --non-interactive\n").arg(script));
    m_process->start(QStringLiteral("/bin/bash"),
                     {script, QStringLiteral("--rosetta"), QStringLiteral("--non-interactive"),
                      QStringLiteral("--app"), appBundlePath()});
}

void ToolchainSetupDialog::onOutput()
{
    m_pending += m_process->readAllStandardOutput();
    int nl;
    while ((nl = m_pending.indexOf('\n')) >= 0) {
        const QString line = QString::fromUtf8(m_pending.left(nl)).trimmed();
        m_pending.remove(0, nl + 1);
        if (line.isEmpty()) continue;

        m_log->appendPlainText(line);
        if (line.startsWith(QLatin1String("==> "))) {
            const QString step = line.mid(4);
            m_step->setText(step);
            for (int i = 0; i < kStepCount; ++i)
                if (step.startsWith(QLatin1String(kSteps[i])))
                    m_progress->setValue(qMax(m_progress->value(), i));
        } else if (line.startsWith(QStringLiteral("✘"))) {
            m_lastError = line.mid(1).trimmed();
        } else if (line.contains(QLatin1String("downloading")) || line.contains(QLatin1String("extracting"))
                   || line.contains(QLatin1String("cloning")) || line.contains(QLatin1String("building"))
                   || line.contains(QLatin1String("launching the installer"))) {
            m_step->setText(m_step->text().section(QStringLiteral(" — "), 0, 0)
                            + QStringLiteral(" — ") + line);
        }
    }
}

void ToolchainSetupDialog::onFinished(int exitCode, bool crashed)
{
    if (m_process) {
        m_pending += m_process->readAllStandardOutput();
        m_pending += '\n';
        onOutput();
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_cancel->hide();

    const bool ok = !m_cancelled && !crashed && exitCode == 0;
    if (ok) {
        m_progress->setValue(kStepCount);
        m_step->setText(tr("The toolchain is installed."));
        m_close->show();
        m_close->setDefault(true);
    } else if (m_cancelled) {
        m_step->setText(tr("Cancelled. Run it again from Tools ▸ Install / Update Toolchain…"));
        m_close->show();
    } else {
        if (installerScript().isEmpty())
            m_lastError = tr("The installer is not bundled with this copy of KayteIDE.");
        m_step->setText(tr("Installation failed: %1")
                            .arg(m_lastError.isEmpty() ? tr("see the details") : m_lastError));
        m_details->setChecked(true);
        m_retry->show();
        m_close->show();
    }
    emit finished(ok, m_cancelled);
}

bool ToolchainSetupDialog::confirmCancel()
{
    return QMessageBox::question(this, windowTitle(),
               tr("Stop installing the toolchain? You can run it again later from "
                  "Tools ▸ Install / Update Toolchain…")) == QMessageBox::Yes;
}

void ToolchainSetupDialog::stopInstaller()
{
    if (!m_process || m_process->state() == QProcess::NotRunning) return;
    m_cancelled = true;
    m_cancel->setEnabled(false);
    m_step->setText(tr("Stopping…"));
#ifdef Q_OS_UNIX
    // The whole group: the script traps TERM and cleans up its temp files.
    ::kill(-pid_t(m_process->processId()), SIGTERM);
#else
    m_process->terminate();
#endif
    if (!m_process->waitForFinished(8000)) {
#ifdef Q_OS_UNIX
        ::kill(-pid_t(m_process->processId()), SIGKILL);
#endif
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

void ToolchainSetupDialog::closeEvent(QCloseEvent *event)
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        // Closing the window doesn't stop the install; hide it instead.
        hide();
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}
