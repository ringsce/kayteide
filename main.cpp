#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QLocale>

// Your custom JSON translator header
#include "myjsontranslator.h"
// Header for your MainWindow class
#include "mainwindow.h"

// This helper function (readLinesWithQt) is not used in your main()
// If it's not used elsewhere in your project for debugging or logging,
// you might consider moving it to a utility class/file or removing it
// to keep main.cpp focused solely on application startup.
void readLinesWithQt(const QString& filename) {
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Could not open file" << filename << ":" << file.errorString();
        return;
    }

    QTextStream in(&file);
    QString line;
    int lineNumber = 0;

    while (!in.atEnd()) {
        line = in.readLine();
        lineNumber++;
        qDebug() << "Line" << lineNumber << ":" << line;
    }

    file.close();
}

int main(int argc, char *argv[])
{
    // 1. Create a QApplication object. This must be the first Qt object instantiated.
    QApplication a(argc, argv);

    // --- Internationalization Setup ---
    // Detect the system's preferred language.
    QString localeName = QLocale::system().name(); // e.g., "en_US", "es_ES", "pt_PT"
    QString languageCode = localeName.split('_').first(); // Extracts "en", "es", "pt"

    // Create an instance of your custom JSON translator on the heap,
    // with 'a' (the QApplication) as its parent.
    // This ensures the translator is automatically destroyed when the application exits.
    MyJsonTranslator *translator = new MyJsonTranslator(&a);

    // Attempt to load the translation file for the detected language.
    QString translationFilePath = QString(":/i18n/%1.json").arg(languageCode); // e.g., ":/i18n/es.json"
    if (translator->load(translationFilePath)) {
        a.installTranslator(translator);
        qDebug() << "Loaded translator for locale:" << languageCode;
    } else {
        qWarning() << "Failed to load specific translation for language code:" << languageCode
                   << ". Attempting to load default (English).";
        // Fallback to English if the specific language translation is not found.
        if (translator->load(":/i18n/en.json")) {
            a.installTranslator(translator);
            qDebug() << "Loaded default English translator.";
        } else {
            qWarning() << "Failed to load default English translation. No translations will be active.";
            // If even the default English translation fails, delete the translator object
            // as it won't be used and will just occupy memory.
            delete translator;
            translator = nullptr; // Set to nullptr to avoid dangling pointer issues
        }
    }
    // --- End Internationalization Setup ---


    // --- Splash Screen Setup ---
    // Load your image for the splash screen.
    // Ensure the path matches your resources.qrc entry for kayte.png.
    // Example: If qrc has <file>images/kayte.png</file>, path is ":/images/kayte.png"
    // If qrc has <file>kayte.png</file>, path is ":/kayte.png" (as used here).
    QPixmap pixmap(":/kayte.png");

    // Check if the pixmap loaded successfully.
    // It's good practice to provide a fallback or warning if the image is missing.
    if (pixmap.isNull()) {
        qWarning("Failed to load splash screen image 'kayte.png'. Proceeding without a visual splash screen.");
        // You could potentially create a blank splash screen or just skip it if the image is critical.
    }

    // Create the splash screen instance with the loaded pixmap.
    QSplashScreen splash(pixmap);

    // Optional: Add a message to the splash screen.
    // Use QObject::tr() to make this message translatable by your MyJsonTranslator.
    splash.showMessage(QObject::tr("Loading Kayte IDE..."), Qt::AlignBottom | Qt::AlignCenter, Qt::white);

    // Show the splash screen.
    splash.show();

    // Optional: Simulate some loading time to ensure the splash screen is visible
    // for a minimum duration, even if the main window loads very quickly.
    QElapsedTimer timer;
    timer.start();
    // Process events to allow the splash screen to render and respond to OS events
    // while we wait, preventing it from appearing frozen.
    while (timer.elapsed() < 2000) { // Keep splash visible for at least 2 seconds
        a.processEvents();
    }
    // --- End Splash Screen Setup ---


    // 2. Create an instance of your MainWindow.
    // This is where the main application window's constructor runs,
    // which might involve more setup. The splash screen remains visible during this.
    MainWindow w;

    // 3. Show the main window.
    w.show();

    // 4. Hide the splash screen gracefully once the main window is fully shown.
    // The splash screen will fade out or disappear immediately depending on the platform.
    splash.finish(&w);

    // 5. Enter the main event loop and hand over control to Qt.
    // The application will run until a.exit() is called or the last window is closed.
    return a.exec();
}