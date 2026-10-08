#include "kaytecli.h"
#include "buildconfig.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QXmlStreamReader>

#include <cstdio>

namespace {

void out(const QString &s) { std::fputs(qPrintable(s + QLatin1Char('\n')), stdout); std::fflush(stdout); }
void err(const QString &s) { std::fputs(qPrintable(QStringLiteral("kayte: ") + s + QLatin1Char('\n')), stderr); }

struct KayteProject
{
    QString dir;                 // folder holding the .xproj
    QString file;                // the .xproj itself
    QString name;
    QString sourceDir  = QStringLiteral("src");
    QString buildDir   = QStringLiteral("build");
    QStringList files;           // <Files><File>, relative to `dir`

    QString path(const QString &rel) const { return QDir(dir).absoluteFilePath(rel); }
};

bool loadProject(const QString &arg, KayteProject *p, QString *error)
{
    QFileInfo fi(arg.isEmpty() ? QDir::currentPath() : arg);
    if (fi.isDir()) {
        const QStringList xprojs = QDir(fi.absoluteFilePath())
                                       .entryList({QStringLiteral("*.xproj")}, QDir::Files, QDir::Name);
        if (xprojs.isEmpty()) {
            *error = QStringLiteral("no .xproj file in %1").arg(fi.absoluteFilePath());
            return false;
        }
        fi.setFile(QDir(fi.absoluteFilePath()).absoluteFilePath(xprojs.first()));
    }
    QFile f(fi.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("cannot open %1").arg(fi.absoluteFilePath());
        return false;
    }

    p->file = fi.absoluteFilePath();
    p->dir  = fi.absolutePath();
    p->name = fi.completeBaseName();

    QXmlStreamReader xml(&f);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement) continue;
        const auto tag = xml.name();
        if (tag == QLatin1String("XProject") || tag == QLatin1String("Files")) continue;
        const QString text = xml.readElementText().trimmed();
        if (text.isEmpty()) continue;
        if (tag == QLatin1String("Name"))           p->name = text;
        else if (tag == QLatin1String("SourceDir")) p->sourceDir = text;
        else if (tag == QLatin1String("BuildDir"))  p->buildDir = text;
        else if (tag == QLatin1String("File"))      p->files << text;
    }
    if (xml.hasError()) {
        *error = QStringLiteral("%1: %2").arg(p->file, xml.errorString());
        return false;
    }
    return true;
}

bool isKayteSource(const QString &file)
{
    const QString suffix = QFileInfo(file).suffix().toLower();
    return suffix == QLatin1String("kayte") || suffix == QLatin1String("kyt")
        || suffix == QLatin1String("kjs");
}

// The program to compile: the first Kayte file listed under <Files>, else
// <SourceDir>/main.kayte.
QString mainSource(const KayteProject &p)
{
    for (const QString &f : p.files)
        if (isKayteSource(f)) return p.path(f);
    for (const QString &candidate : {p.sourceDir + QStringLiteral("/main.kayte"),
                                     p.sourceDir + QStringLiteral("/main.kjs")})
        if (QFileInfo::exists(p.path(candidate))) return p.path(candidate);
    return {};
}

// The Kayte compiler: the SDK bundled in Contents/tools/kayte, else one on
// PATH (but never this driver itself).
QString findCompiler()
{
    const QString tools = BuildConfigurations::toolsDir();
    if (!tools.isEmpty() && QFileInfo(tools + QStringLiteral("/kayte/bin/kayte")).isExecutable())
        return tools + QStringLiteral("/kayte/bin/kayte");
    const QString self = QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath();
    const QString onPath = QStandardPaths::findExecutable(QStringLiteral("kayte"));
    if (!onPath.isEmpty() && QFileInfo(onPath).canonicalFilePath() != self)
        return onPath;
    return {};
}

QString outputDir(const KayteProject &p, const QString &config)
{
    return QDir::cleanPath(p.path(p.buildDir) + QLatin1Char('/') + config);
}

QString executable(const KayteProject &p, const QString &config)
{
    return outputDir(p, config) + QLatin1Char('/') + p.name;
}

// Runs the Kayte compiler on `source`, printing its output without the banner
// and with errors in the GCC form ("file:2:25: error: msg") that IDEs turn
// into clickable problems — the compiler itself reports "Parser Error: msg
// at 2:25 (Token: …)" or "… at line 3" without a file name.
int runCompiler(const QString &compiler, const QStringList &args,
                const QString &workingDir, const QString &source)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    if (!workingDir.isEmpty()) proc.setWorkingDirectory(workingDir);
    proc.start(compiler, args);
    if (!proc.waitForStarted()) {
        err(QStringLiteral("could not start %1").arg(compiler));
        return 2;
    }

    static const QRegularExpression located(QStringLiteral(
        R"x(^(?:\w+ )?Error: (.+?)\s+at (?:line )?(\d+)(?::(\d+))?(?:\s+\(.*\))?\s*$)x"));
    static const QRegularExpression banner(QStringLiteral(
        R"x(^(Kayte Language Runtime Environment|=+)\s*$)x"));
    QByteArray pending;
    const auto flush = [&](bool all) {
        int nl;
        while ((nl = pending.indexOf('\n')) >= 0 || (all && !pending.isEmpty())) {
            const int len = nl >= 0 ? nl : int(pending.size());
            QString line = QString::fromLocal8Bit(pending.left(len)).trimmed();
            pending.remove(0, nl >= 0 ? nl + 1 : len);
            if (line.isEmpty() || banner.match(line).hasMatch()) continue;
            if (const auto m = located.match(line); m.hasMatch() && !source.isEmpty()) {
                line = m.captured(3).isEmpty()
                    ? QStringLiteral("%1:%2: error: %3").arg(source, m.captured(2), m.captured(1))
                    : QStringLiteral("%1:%2:%3: error: %4")
                          .arg(source, m.captured(2), m.captured(3), m.captured(1));
            }
            out(line);
        }
    };
    while (proc.state() != QProcess::NotRunning) {
        proc.waitForReadyRead(200);
        pending += proc.readAll();
        flush(false);
    }
    pending += proc.readAll();
    flush(true);
    return proc.exitStatus() == QProcess::NormalExit ? proc.exitCode() : 1;
}

int build(const KayteProject &p, const QString &config)
{
    const QString main = mainSource(p);
    if (main.isEmpty()) {
        err(QStringLiteral("no main source: list one under <Files> or add %1/main.kayte")
                .arg(p.sourceDir));
        return 2;
    }
    const QString compiler = findCompiler();
    if (compiler.isEmpty()) {
        err(QStringLiteral("the Kayte SDK is not installed (run src/scripts/requirements.sh "
                           "to bundle it in KayteIDE.app)"));
        return 2;
    }

    const QString outDir = outputDir(p, config);
    QDir().mkpath(outDir);

    // Native executable through C; Debug keeps the generated C next to it.
    QStringList args{QStringLiteral("--native"), main, QStringLiteral("-o"), executable(p, config)};
    if (config.compare(QLatin1String("Release"), Qt::CaseInsensitive) != 0)
        args << QStringLiteral("--keep-c");

    out(QStringLiteral("kayte: building %1 (%2)").arg(p.name, config));
    const int rc = runCompiler(compiler, args, QFileInfo(main).absolutePath(), main);
    if (rc != 0 || !QFileInfo(executable(p, config)).isExecutable()) {
        err(QStringLiteral("build failed"));
        return 1;
    }
    out(QStringLiteral("kayte: built %1").arg(executable(p, config)));
    return 0;
}

bool upToDate(const KayteProject &p, const QString &config)
{
    const QFileInfo exe(executable(p, config));
    if (!exe.exists()) return false;
    const QDateTime built = exe.lastModified();
    return QFileInfo(p.file).lastModified() <= built
        && QFileInfo(mainSource(p)).lastModified() <= built;
}

int runExecutable(const QString &program, const QStringList &args, const QString &workingDir)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.setInputChannelMode(QProcess::ForwardedInputChannel);
    if (!workingDir.isEmpty()) proc.setWorkingDirectory(workingDir);
    proc.start(program, args);
    if (!proc.waitForStarted(-1)) {
        err(QStringLiteral("could not start %1").arg(program));
        return 2;
    }
    proc.waitForFinished(-1);
    return proc.exitStatus() == QProcess::NormalExit ? proc.exitCode() : 1;
}

int run(const KayteProject &p, const QString &config, const QStringList &programArgs)
{
    if (!upToDate(p, config))
        if (const int rc = build(p, config); rc != 0) return rc;
    return runExecutable(executable(p, config), programArgs, p.dir);
}

int clean(const KayteProject &p, const QString &config, bool all)
{
    const QString target = all ? QDir::cleanPath(p.path(p.buildDir)) : outputDir(p, config);
    // Never delete anything outside (or equal to) the project folder.
    const QString root = QDir::cleanPath(p.dir) + QLatin1Char('/');
    if (!target.startsWith(root)) {
        err(QStringLiteral("refusing to delete %1: not inside the project").arg(target));
        return 2;
    }
    if (QFileInfo::exists(target) && !QDir(target).removeRecursively()) {
        err(QStringLiteral("could not remove %1").arg(target));
        return 1;
    }
    out(QStringLiteral("kayte: removed %1").arg(target));
    return 0;
}

} // namespace

bool isKayteCliInvocation(const char *argv0)
{
    return argv0 && QFileInfo(QString::fromLocal8Bit(argv0)).baseName()
                        .compare(QLatin1String("kayte"), Qt::CaseInsensitive) == 0;
}

int kayteCliMain(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kayte"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    QStringList args = app.arguments();
    const QString command = args.value(1);
    const bool projectCommand = command == QLatin1String("build")
                             || command == QLatin1String("run")
                             || command == QLatin1String("clean");

    // Not a project command: hand everything to the Kayte compiler itself.
    if (!projectCommand && command != QLatin1String("--help") && command != QLatin1String("-h")) {
        const QString compiler = findCompiler();
        if (compiler.isEmpty()) {
            err(QStringLiteral("the Kayte SDK is not installed (run src/scripts/requirements.sh)"));
            return 2;
        }
        // Compiling (not running a program or the REPL): name the source file
        // in error messages too.
        const QStringList passed = args.mid(1);
        static const QStringList compileModes{
            QStringLiteral("--compile"), QStringLiteral("--native"),
            QStringLiteral("--llvm"), QStringLiteral("--native-arm64")};
        bool compiling = false;
        QString source;
        for (const QString &a : passed) {
            if (compileModes.contains(a)) compiling = true;
            else if (source.isEmpty() && isKayteSource(a)) source = QFileInfo(a).absoluteFilePath();
        }
        if (compiling)
            return runCompiler(compiler, passed, QString(), source);
        return runExecutable(compiler, passed, QString());
    }

    // Everything after "--" belongs to the program started by `kayte run`.
    QStringList programArgs;
    if (const int dashes = args.indexOf(QStringLiteral("--")); dashes > 0) {
        programArgs = args.mid(dashes + 1);
        args = args.mid(0, dashes);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Builds and runs Kayte projects (.xproj) with the Kayte SDK bundled in KayteIDE.\n"
        "Other arguments (--compile, --native, --run, --repl, …) go to the Kayte compiler."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("command"), QStringLiteral("build, run or clean"));
    parser.addPositionalArgument(QStringLiteral("project"),
                                 QStringLiteral(".xproj file or its folder (default: current folder)"),
                                 QStringLiteral("[project]"));
    const QCommandLineOption configOpt({QStringLiteral("c"), QStringLiteral("config")},
                                       QStringLiteral("Build configuration (Debug or Release)."),
                                       QStringLiteral("name"), QStringLiteral("Debug"));
    const QCommandLineOption allOpt(QStringLiteral("all"),
                                    QStringLiteral("clean: remove the whole build folder."));
    parser.addOption(configOpt);
    parser.addOption(allOpt);
    parser.process(args);

    const QStringList pos = parser.positionalArguments();
    if (pos.isEmpty() || pos.size() > 2) parser.showHelp(2);

    KayteProject project;
    QString error;
    if (!loadProject(pos.value(1), &project, &error)) {
        err(error);
        return 2;
    }

    const QString config = parser.value(configOpt);
    if (command == QLatin1String("build")) return build(project, config);
    if (command == QLatin1String("run"))   return run(project, config, programArgs);
    return clean(project, config, parser.isSet(allOpt));
}
