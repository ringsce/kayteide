#include "LanguageRunner.h"

LanguageRunner LanguageRunner::fromFile(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    LanguageRunner r;

    if (ext == "kayte") {
        r.language    = "Kayte";
        r.icon        = "🔮";
        r.mode        = Mode::Interpreted;
        r.interpreter = "kayte";
    }
    else if (ext == "php") {
        r.language    = "PHP";
        r.icon        = "🐘";
        r.mode        = Mode::Interpreted;
        r.interpreter = "php";
        r.interpreterFlags = {"-f"};
    }
    else if (ext == "pl") {
        r.language    = "Perl";
        r.icon        = "🦪";
        r.mode        = Mode::Interpreted;
        r.interpreter = "perl";
        r.interpreterFlags = {"-w"};
    }
    else if (ext == "pas") {
        r.language    = "Pascal";
        r.icon        = "📐";
        r.mode        = Mode::Compiled;
        r.compiler    = "fpc";
        r.compilerFlags = {"-v0"};
    }
    else if (ext == "c") {
        r.language    = "C";
        r.icon        = "⚙️";
        r.mode        = Mode::Compiled;
        r.compiler    = "gcc";
        r.compilerFlags = {"-Wall", "-Wextra", "-o"};
    }
    else if (ext == "cpp" || ext == "cxx" || ext == "cc") {
        r.language    = "C++";
        r.icon        = "⚡";
        r.mode        = Mode::Compiled;
        r.compiler    = "g++";
        r.compilerFlags = {"-Wall", "-Wextra", "-std=c++17", "-o"};
    }
    else if (ext == "h" || ext == "hpp" || ext == "hxx") {
        r.language    = "Header";
        r.icon        = "📋";
        r.mode        = Mode::HeaderOnly;
        r.compiler    = (ext == "h") ? "gcc" : "g++";
        // Compile as header unit to catch errors
        r.compilerFlags = {"-Wall", "-Wextra", "-fsyntax-only", "-x",
                           (ext == "h") ? "c-header" : "c++-header"};
    }
    else {
        // Fallback: treat as QTest binary
        r.language = "QTest Binary";
        r.icon     = "🧪";
        r.mode     = Mode::QtTest;
    }

    return r;
}

bool LanguageRunner::isSupported(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    return supportedExtensions().contains(ext) || QFileInfo(path).suffix().isEmpty();
}

QStringList LanguageRunner::supportedExtensions()
{
    return {"kayte", "php", "pl", "pas", "c", "cpp", "cxx", "cc",
            "h", "hpp", "hxx"};
}
