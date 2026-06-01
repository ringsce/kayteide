#pragma once
#include <QString>
#include <QStringList>
#include <QFileInfo>

// Describes how to handle a given file type.
struct LanguageRunner {
    QString language;       // display name
    QString icon;           // emoji / unicode for tree display

    // Returns the command + args to execute for this file.
    // For compiled languages, this is the compile step; run step handled separately.
    // For interpreted languages this is the direct run command.
    enum class Mode {
        Interpreted,    // interpreter runs the file directly
        Compiled,       // needs compile → run two-step
        QtTest,         // compiled QTest binary (existing behaviour)
        HeaderOnly      // just lint/analyse, no run
    } mode = Mode::Interpreted;

    QString interpreter;        // e.g. "php", "perl", "python3", "kayte"
    QString compiler;           // e.g. "gcc", "g++", "fpc"
    QStringList compilerFlags;  // extra flags
    QStringList interpreterFlags;

    static LanguageRunner fromFile(const QString &path);
    static bool isSupported(const QString &path);
    static QStringList supportedExtensions();
};
