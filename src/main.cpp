#include <QApplication>
#include <QLocale>
#include <QDebug>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QDir>
#include <QDockWidget>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStyleFactory>
#include <QSurfaceFormat>

#include <iostream>
#include <fstream>

#include "mainwindow.h"
#include "myjsontranslator.h"

#ifdef KAYTE_GIT_CLIENT_ENABLED
#  include "GitClientPanel.hpp"
#endif

int main(int argc, char *argv[])
{
    // ── High-DPI & surface format (must precede QApplication) ────────────────
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    // ── Application object ───────────────────────────────────────────────────
    QApplication a(argc, argv);
    a.setApplicationName("KayteIDE");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("ringsce");
    a.setOrganizationDomain("ringsce.com");
    a.setStyle(QStyleFactory::create("Fusion"));

    // ── Command-line parsing ─────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("KayteIDE – The Kayte Language IDE");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("path",
        QObject::tr("Project folder or source file to open at startup."),
        "[path]");

    QCommandLineOption repoOption(
        {"r", "repo"},
        QObject::tr("Git repository to open in the Git panel."),
        "repo-path");
    parser.addOption(repoOption);

    parser.process(a);

    const QStringList positionalArgs = parser.positionalArguments();
    const QString     startupPath    = positionalArgs.isEmpty()
                                       ? QDir::currentPath()
                                       : positionalArgs.first();
    const QString     repoPath       = parser.isSet(repoOption)
                                       ? parser.value(repoOption)
                                       : startupPath;
    Q_UNUSED(repoPath) // used only when KAYTE_GIT_CLIENT_ENABLED

    // ── Internationalisation ─────────────────────────────────────────────────
    const QString languageCode = QLocale::system().name().split('_').first();

    auto *translator = new MyJsonTranslator(&a);

    auto tryLoad = [&](const QString &code) -> bool {
        const QString path = QString(":/i18n/%1.json").arg(code);
        if (translator->load(path)) {
            a.installTranslator(translator);
            qDebug() << "Loaded translation:" << path;
            return true;
        }
        return false;
    };

    if (!tryLoad(languageCode) && !tryLoad("en")) {
        qWarning() << "No translations loaded – running with built-in strings.";
        delete translator;
        translator = nullptr;
    }

    // ── Splash screen ────────────────────────────────────────────────────────
    QPixmap splashPixmap(":/kayte.png");
    if (splashPixmap.isNull())
        qWarning("Splash image 'kayte.png' not found – skipping splash.");

    QScopedPointer<QSplashScreen> splash;
    if (!splashPixmap.isNull()) {
        splash.reset(new QSplashScreen(splashPixmap));
        splash->showMessage(
            QObject::tr("Loading KayteIDE…"),
            Qt::AlignBottom | Qt::AlignCenter,
            Qt::white);
        splash->show();
        a.processEvents(); // paint the splash before the window loads
    }

    // ── Main window ──────────────────────────────────────────────────────────
    MainWindow w;

    // ── Git client dock ──────────────────────────────────────────────────────
#ifdef KAYTE_GIT_CLIENT_ENABLED
    auto *gitPanel = new Kayte::GitClientPanel(&w);

    QObject::connect(gitPanel, &Kayte::GitClientPanel::statusMessage,
                     &w, [&w](const QString &msg) {
        if (w.statusBar())
            w.statusBar()->showMessage(msg, 5000);
    });

    auto *gitDock = new QDockWidget(QObject::tr("Git"), &w);
    gitDock->setObjectName("GitClientDock");
    gitDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    gitDock->setWidget(gitPanel);
    w.addDockWidget(Qt::BottomDockWidgetArea, gitDock);

    QTimer::singleShot(0, gitPanel, [gitPanel, repoPath] {
        if (!repoPath.isEmpty())
            gitPanel->openRepository(repoPath);
    });
#endif // KAYTE_GIT_CLIENT_ENABLED

    // ── Open startup file/folder in the editor (if provided) ─────────────────
    if (!positionalArgs.isEmpty()) {
        QTimer::singleShot(0, &w, [&w, startupPath] {
            // Call whatever open method MainWindow exposes, e.g.:
            // w.openPath(startupPath);
            Q_UNUSED(startupPath)
        });
    }

    // ── Show & finish splash ─────────────────────────────────────────────────
    w.show();

    if (splash)
        splash->finish(&w);

    // ── Event loop ───────────────────────────────────────────────────────────
    return a.exec();
}
