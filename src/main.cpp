#include <QApplication>
#include <QLocale>
#include <QDebug>
#include <QSplashScreen>
#include <QPixmap>
#include <QElapsedTimer>

#include <iostream> // For std::cin, std::cout, std::cerr
#include <fstream>  // For std::ifstream, std::ofstream

#include "mainwindow.h"       // Assuming your main window class
#include "myjsontranslator.h" // Assuming your translator class

int main(int argc, char *argv[])
{
    // 1. Create a QApplication object. This must be the first Qt object instantiated.
    QApplication a(argc, argv);

    // --- Internationalization Setup ---
    // ... (Your existing internationalization code) ...
    QString localeName = QLocale::system().name();
    QString languageCode = localeName.split('_').first();

    MyJsonTranslator *translator = new MyJsonTranslator(&a);

    QString translationFilePath = QString(":/i18n/%1.json").arg(languageCode);
    if (translator->load(translationFilePath)) {
        a.installTranslator(translator);
        qDebug() << "Loaded translator for locale:" << languageCode;
    } else {
        qWarning() << "Failed to load specific translation for language code:" << languageCode
                   << ". Attempting to load default (English).";
        if (translator->load(":/i18n/en.json")) {
            a.installTranslator(translator);
            qDebug() << "Loaded default English translator.";
        } else {
            qWarning() << "Failed to load default English translation. No translations will be active.";
            delete translator;
            translator = nullptr;
        }
    }
    // --- End Internationalization Setup ---


    // --- Splash Screen Setup ---
    // ... (Your existing splash screen code) ...
    QPixmap pixmap(":/kayte.png");
    if (pixmap.isNull()) {
        qWarning("Failed to load splash screen image 'kayte.png'. Proceeding without a visual splash screen.");
    }

    QSplashScreen splash(pixmap);
    splash.showMessage(QObject::tr("Loading Kayte IDE..."), Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    splash.show();

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 2000) {
        a.processEvents();
    }
    // --- End Splash Screen Setup ---

    // 2. Create an instance of your MainWindow.
    MainWindow w;

    // 3. Show the main window.
    w.show();

    // 4. Hide the splash screen gracefully once the main window is fully shown.
    splash.finish(&w);

    // 5. Enter the main event loop and hand over control to Qt.
    return a.exec();
}