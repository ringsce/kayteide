#include "buildconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSysInfo>

#include <utility>

namespace {

QString readHead(const QString &path, qint64 maxBytes = 256 * 1024)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.read(maxBytes));
}

QString quoted(const QString &s) { return QLatin1Char('"') + s + QLatin1Char('"'); }

// A bundled tool when the app ships one in Contents/tools/bin, else the
// plain name from PATH.
QString tool(const QString &name)
{
    const QString dir = BuildConfigurations::toolsDir();
    if (!dir.isEmpty() && QFileInfo(dir + QStringLiteral("/bin/") + name).isExecutable())
        return quoted(QStringLiteral("${KayteTools}/bin/") + name);
    return name;
}

BuildConfiguration make(const QString &name, const QString &buildDir,
                        const QString &build, const QString &clean,
                        const QString &run, const QString &debug)
{
    return {name, buildDir, build, clean, run, debug};
}

QStringList filesIn(const QString &dir, const QStringList &patterns)
{
    return QDir(dir).entryList(patterns, QDir::Files, QDir::Name);
}

// ── CMake ────────────────────────────────────────────────────────────────────
QVector<BuildConfiguration> detectCMake(const QString &dir, const QString &name)
{
    const QString text = readHead(dir + QStringLiteral("/CMakeLists.txt"));

    QString target = name;
    const auto exe = QRegularExpression(QStringLiteral(R"(add_executable\s*\(\s*([^\s)]+))"),
                                        QRegularExpression::CaseInsensitiveOption).match(text);
    if (exe.hasMatch()) target = exe.captured(1);
    if (target.contains(QLatin1String("PROJECT_NAME"))) {
        const auto proj = QRegularExpression(QStringLiteral(R"(project\s*\(\s*([^\s)]+))"),
                                             QRegularExpression::CaseInsensitiveOption).match(text);
        target = proj.hasMatch() ? proj.captured(1) : name;
    }
    if (target.contains(QLatin1Char('$'))) target = name;

    QString exePath = QStringLiteral("${BuildDir}/") + target;
#ifdef Q_OS_MACOS
    if (text.contains(QLatin1String("MACOSX_BUNDLE")))
        exePath = QStringLiteral("${BuildDir}/%1.app/Contents/MacOS/%1").arg(target);
#endif

    QVector<BuildConfiguration> out;
    for (const QString &type : {QStringLiteral("Debug"), QStringLiteral("Release")}) {
        out << make(type, QStringLiteral("build/") + type,
                    QStringLiteral(R"(cmake -S "${ProjectDir}" -B "${BuildDir}" -DCMAKE_BUILD_TYPE=%1 && cmake --build "${BuildDir}" --parallel)").arg(type),
                    QStringLiteral(R"(cmake --build "${BuildDir}" --target clean)"),
                    quoted(exePath),
                    QStringLiteral("lldb -- ") + quoted(exePath));
    }
    return out;
}

// ── Lazarus (.lpi) ───────────────────────────────────────────────────────────
QVector<BuildConfiguration> detectLazarus(const QString &dir, const QString &lpi)
{
    const QString text = readHead(dir + QLatin1Char('/') + lpi);

    QString exe = QFileInfo(lpi).completeBaseName();
    const auto target = QRegularExpression(
        QStringLiteral(R"x(<Target>\s*<Filename\s+Value="([^"]+)")x")).match(text);
    if (target.hasMatch() && !target.captured(1).contains(QLatin1Char('$')))
        exe = target.captured(1);
    const QString exePath = exe.startsWith(QLatin1Char('/')) ? exe : QStringLiteral("./") + exe;

    // Clean removes the project's unit output folder (Lazarus' default is
    // lib/$(TargetCPU)-$(TargetOS), often written with a backslash).
    QString clean;
    const auto units = QRegularExpression(
        QStringLiteral(R"x(<UnitOutputDirectory\s+Value="([^"]+)")x")).match(text);
    if (units.hasMatch()) {
        QString cpu = QSysInfo::currentCpuArchitecture();
        if (cpu == QLatin1String("arm64")) cpu = QStringLiteral("aarch64");
#if defined(Q_OS_MACOS)
        const QString os = QStringLiteral("darwin");
#elif defined(Q_OS_WIN)
        const QString os = QStringLiteral("win64");
#else
        const QString os = QStringLiteral("linux");
#endif
        QString out = units.captured(1);
        out.replace(QLatin1Char('\\'), QLatin1Char('/'))
           .replace(QLatin1String("$(TargetCPU)"), cpu)
           .replace(QLatin1String("$(TargetOS)"), os);
        out = QDir::cleanPath(out);
        // Only a plain sub-folder of the project — never ".", "..", or absolute.
        if (!out.contains(QLatin1Char('$')) && !QDir::isAbsolutePath(out)
            && out != QLatin1String(".") && !out.startsWith(QLatin1String("..")))
            clean = QStringLiteral("rm -rf %1").arg(quoted(out));
    }

    const QString lazbuild = tool(QStringLiteral("lazbuild"));
    const auto hasMode = [&](const QString &mode) {
        return text.contains(QStringLiteral("<Item Name=\"%1\"").arg(mode))
            || text.contains(QStringLiteral("Name=\"%1\"").arg(mode));
    };
    const QString debugBuild = hasMode(QStringLiteral("Debug"))
        ? QStringLiteral("%1 --build-mode=Debug %2").arg(lazbuild, quoted(lpi))
        : QStringLiteral("%1 %2").arg(lazbuild, quoted(lpi));
    const QString releaseBuild = hasMode(QStringLiteral("Release"))
        ? QStringLiteral("%1 --build-mode=Release %2").arg(lazbuild, quoted(lpi))
        : QStringLiteral("%1 --opt=-O2 --opt=-Xs --opt=-g- %2").arg(lazbuild, quoted(lpi));

    return {
        make(QStringLiteral("Debug"),   QStringLiteral("."), debugBuild,   clean,
             quoted(exePath), QStringLiteral("lldb -- ") + quoted(exePath)),
        make(QStringLiteral("Release"), QStringLiteral("."), releaseBuild, clean,
             quoted(exePath), QStringLiteral("lldb -- ") + quoted(exePath)),
    };
}

// ── Plain Free Pascal program ────────────────────────────────────────────────
QString findPascalProgram(const QString &dir)
{
    const QStringList projects = filesIn(dir, {QStringLiteral("*.lpr"), QStringLiteral("*.dpr")});
    if (!projects.isEmpty()) return projects.first();

    static const QRegularExpression program(QStringLiteral(R"(^\s*program\s+\w+)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
    for (const QString &f : filesIn(dir, {QStringLiteral("*.pas"), QStringLiteral("*.pp")}))
        if (program.match(readHead(dir + QLatin1Char('/') + f, 8192)).hasMatch())
            return f;
    return {};
}

QVector<BuildConfiguration> detectPascal(const QString &main)
{
    const QString fpc = tool(QStringLiteral("fpc"));
    const QString exe = QStringLiteral("${BuildDir}/") + QFileInfo(main).completeBaseName();
    const QString compile = QStringLiteral(
        R"(mkdir -p "${BuildDir}/units" && %1 %2 -FE"${BuildDir}" -FU"${BuildDir}/units" %3)");
    return {
        make(QStringLiteral("Debug"), QStringLiteral("build/Debug"),
             compile.arg(fpc, QStringLiteral("-g -gl -O-"), quoted(main)),
             QStringLiteral(R"(rm -rf "${BuildDir}")"),
             quoted(exe), QStringLiteral("lldb -- ") + quoted(exe)),
        make(QStringLiteral("Release"), QStringLiteral("build/Release"),
             compile.arg(fpc, QStringLiteral("-O2 -Xs"), quoted(main)),
             QStringLiteral(R"(rm -rf "${BuildDir}")"),
             quoted(exe), QStringLiteral("lldb -- ") + quoted(exe)),
    };
}

// ── Kayte in a Linux VM (Apple container) ────────────────────────────────────
// Run builds a fresh Linux VM from the bundled Kayte SDK, compiles the program
// there, runs it, and deletes the VM, images and temp files afterwards.
QVector<BuildConfiguration> kayteContainerConfigs(const QString &program)
{
    const QString runner = tool(QStringLiteral("kayte-container-run"));
    QVector<BuildConfiguration> out;
    for (const auto &[arch, name] : {std::pair{QStringLiteral("arm64"), QStringLiteral("Linux arm64 (container)")},
                                     std::pair{QStringLiteral("amd64"), QStringLiteral("Linux amd64 (Rosetta 2)")}}) {
        const QString base = QStringLiteral("%1 --arch %2").arg(runner, arch);
        // Run already rebuilds the VM, compiler and program from scratch.
        BuildConfiguration c = make(name, QStringLiteral("build/linux-") + arch,
                    QStringLiteral("%1 --build-only %2").arg(base, quoted(program)),
                    QStringLiteral(R"(rm -rf "${BuildDir}" && %1 --clean)").arg(runner),
                    QStringLiteral("%1 %2").arg(base, quoted(program)),
                    // gdb inside the VM; the Debug Console feeds it commands
                    QStringLiteral("%1 --debug %2").arg(base, quoted(program)));
        c.buildBeforeRun = false;
        out << c;
    }
    return out;
}

// ── Kayte (.xproj, built by the bundled `kayte` tool) ────────────────────────
QVector<BuildConfiguration> detectKayte(const QString &dir, const QString &xproj)
{
    const QString text = readHead(dir + QLatin1Char('/') + xproj);
    const auto element = [&text](const char *tag, const QString &fallback) {
        const auto m = QRegularExpression(QStringLiteral("<%1>\\s*([^<]+?)\\s*</%1>")
                                              .arg(QLatin1String(tag))).match(text);
        return m.hasMatch() ? m.captured(1) : fallback;
    };
    const QString name     = element("Name", QFileInfo(xproj).completeBaseName());
    const QString buildDir = element("BuildDir", QStringLiteral("build"));

    QString kayte = QStringLiteral("kayte");
    const QString tools = BuildConfigurations::toolsDir();
    if (!tools.isEmpty() && QFileInfo(tools + QStringLiteral("/sdk/usr/bin/kayte")).isExecutable())
        kayte = quoted(QStringLiteral("${KayteTools}/sdk/usr/bin/kayte"));

    QVector<BuildConfiguration> out;
    for (const QString &config : {QStringLiteral("Debug"), QStringLiteral("Release")}) {
        const QString args = QStringLiteral("--config %1 %2").arg(config, quoted(xproj));
        out << make(config, buildDir + QLatin1Char('/') + config,
                    QStringLiteral("%1 build %2").arg(kayte, args),
                    QStringLiteral("%1 clean %2").arg(kayte, args),
                    QStringLiteral("%1 run %2").arg(kayte, args),
                    QStringLiteral("lldb -- ") + quoted(QStringLiteral("${BuildDir}/") + name));
    }
    out << kayteContainerConfigs(xproj);
    return out;
}

// ── Loose Kayte sources (no .xproj) ──────────────────────────────────────────
// The program is main.kayte / main.kjs, the only Kayte file, or — in a folder
// of several programs (like kayte-lang's examples/) — the file being edited.
QVector<BuildConfiguration> detectKayteFiles(const QStringList &sources)
{
    QString file = QStringLiteral("${File}");
    QString base = QStringLiteral("${FileBaseName}");
    for (const QString &main : {QStringLiteral("main.kayte"), QStringLiteral("main.kjs")}) {
        if (sources.contains(main)) { file = main; base = QStringLiteral("main"); break; }
    }
    if (file.startsWith(QLatin1Char('$')) && sources.size() == 1) {
        file = sources.first();
        base = QFileInfo(file).completeBaseName();
    }

    // The driver in sdk/usr/bin adds file names to compiler errors, so they
    // show up in Problems; fall back to the compiler itself.
    QString kayte = QStringLiteral("kayte");
    const QString tools = BuildConfigurations::toolsDir();
    if (!tools.isEmpty() && QFileInfo(tools + QStringLiteral("/sdk/usr/bin/kayte")).isExecutable())
        kayte = quoted(QStringLiteral("${KayteTools}/sdk/usr/bin/kayte"));
    else
        kayte = tool(QStringLiteral("kayte"));

    const QString exe = quoted(QStringLiteral("${BuildDir}/") + base);
    const QString compile = QStringLiteral(R"(mkdir -p "${BuildDir}" && %1 --native %2 -o %3)")
                                .arg(kayte, quoted(file), exe);
    QVector<BuildConfiguration> out{
        make(QStringLiteral("Debug"), QStringLiteral("build/Debug"),
             compile + QStringLiteral(" --keep-c"), QStringLiteral(R"(rm -rf "${BuildDir}")"),
             exe, QStringLiteral("lldb -- ") + exe),
        make(QStringLiteral("Release"), QStringLiteral("build/Release"),
             compile, QStringLiteral(R"(rm -rf "${BuildDir}")"),
             exe, QStringLiteral("lldb -- ") + exe),
    };
    out << kayteContainerConfigs(file);
    return out;
}

} // namespace

// ─── BuildConfiguration ──────────────────────────────────────────────────────

QJsonObject BuildConfiguration::toJson() const
{
    return {
        {QStringLiteral("name"),     name},
        {QStringLiteral("buildDir"), buildDir},
        {QStringLiteral("build"),    build},
        {QStringLiteral("clean"),    clean},
        {QStringLiteral("run"),      run},
        {QStringLiteral("debug"),    debug},
        {QStringLiteral("buildBeforeRun"), buildBeforeRun},
    };
}

BuildConfiguration BuildConfiguration::fromJson(const QJsonObject &o)
{
    return {
        o.value(QStringLiteral("name")).toString(),
        o.value(QStringLiteral("buildDir")).toString(),
        o.value(QStringLiteral("build")).toString(),
        o.value(QStringLiteral("clean")).toString(),
        o.value(QStringLiteral("run")).toString(),
        o.value(QStringLiteral("debug")).toString(),
        o.value(QStringLiteral("buildBeforeRun")).toBool(true),
    };
}

// ─── BuildConfigurations ─────────────────────────────────────────────────────

BuildConfigurations::BuildConfigurations(QObject *parent)
    : QObject(parent)
{
}

QString BuildConfigurations::toolsDir()
{
    const QString env = qEnvironmentVariable("KAYTE_TOOLS");
    if (!env.isEmpty() && QFileInfo(env).isDir()) return env;
    // macOS: KayteIDE.app/Contents/MacOS/../tools for the IDE, and
    // tools/sdk/usr/bin/../../.. for the bundled `kayte` tool.
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QFileInfo fromIde(exeDir + QStringLiteral("/../tools"));
    if (fromIde.isDir()) return fromIde.canonicalFilePath();
    const QFileInfo fromSdk(exeDir + QStringLiteral("/../../.."));
    if (QFileInfo(fromSdk.filePath() + QStringLiteral("/sdk/usr/bin")).isDir())
        return fromSdk.canonicalFilePath();
    return {};
}

QVector<BuildConfiguration> BuildConfigurations::detect(const QString &dir, const QString &name,
                                                       QString *kind)
{
    const auto setKind = [kind](const char *k) { if (kind) *kind = QString::fromLatin1(k); };

    if (const QStringList xproj = filesIn(dir, {QStringLiteral("*.xproj")}); !xproj.isEmpty()) {
        setKind("Kayte");
        return detectKayte(dir, xproj.first());
    }
    if (QFileInfo::exists(dir + QStringLiteral("/CMakeLists.txt"))) {
        setKind("CMake");
        return detectCMake(dir, name);
    }
    if (const QStringList lpi = filesIn(dir, {QStringLiteral("*.lpi")}); !lpi.isEmpty()) {
        setKind("Lazarus");
        return detectLazarus(dir, lpi.first());
    }
    if (const QString main = findPascalProgram(dir); !main.isEmpty()) {
        setKind("Free Pascal");
        return detectPascal(main);
    }
    if (!filesIn(dir, {QStringLiteral("Makefile"), QStringLiteral("makefile"),
                       QStringLiteral("GNUmakefile")}).isEmpty()) {
        setKind("Make");
        return {make(QStringLiteral("Default"), QStringLiteral("."),
                     QStringLiteral("make"), QStringLiteral("make clean"),
                     QStringLiteral("\"./${ProjectName}\""),
                     QStringLiteral("lldb -- \"./${ProjectName}\""))};
    }
    if (const QStringList kayte = filesIn(dir, {QStringLiteral("*.kayte"), QStringLiteral("*.kjs")});
        !kayte.isEmpty()) {
        setKind("Kayte");
        return detectKayteFiles(kayte);
    }
    setKind("Custom");
    return {make(QStringLiteral("Default"), QStringLiteral("."),
                 QStringLiteral("make all"), QStringLiteral("make clean"),
                 QStringLiteral("./output_executable"),
                 QStringLiteral("lldb ./output_executable"))};
}

QString BuildConfigurations::settingsFile() const
{
    return m_dir.isEmpty() ? QString()
                           : m_dir + QStringLiteral("/.kayteide/build.json");
}

void BuildConfigurations::setProject(const QString &dir, const QString &name)
{
    m_dir  = dir;
    m_name = name.isEmpty() ? QFileInfo(dir).fileName() : name;
    m_configs.clear();
    m_active = 0;
    m_kind.clear();

    QFile f(settingsFile());
    if (!m_dir.isEmpty() && f.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        for (const QJsonValue &v : root.value(QStringLiteral("configurations")).toArray()) {
            const BuildConfiguration c = BuildConfiguration::fromJson(v.toObject());
            if (!c.name.isEmpty()) m_configs << c;
        }
        m_kind = root.value(QStringLiteral("kind")).toString();
        const QString active = root.value(QStringLiteral("active")).toString();
        for (int i = 0; i < m_configs.size(); ++i)
            if (m_configs[i].name == active) m_active = i;
    }
    if (m_configs.isEmpty())
        m_configs = detect(m_dir, m_name, &m_kind);

    emit changed();
}

BuildConfiguration BuildConfigurations::active() const
{
    return m_configs.value(m_active);
}

bool BuildConfigurations::setActiveIndex(int index, QString *error)
{
    if (index < 0 || index >= m_configs.size()) return false;
    m_active = index;
    emit changed();
    return save(error);
}

bool BuildConfigurations::setConfigurations(const QVector<BuildConfiguration> &configs,
                                            int active, QString *error)
{
    if (configs.isEmpty()) return false;
    m_configs = configs;
    m_active  = qBound(0, active, int(configs.size()) - 1);
    emit changed();
    return save(error);
}

bool BuildConfigurations::save(QString *error)
{
    if (m_dir.isEmpty()) return true;
    if (!QDir(m_dir).mkpath(QStringLiteral(".kayteide"))) {
        if (error) *error = tr("Cannot create %1").arg(m_dir + QStringLiteral("/.kayteide"));
        return false;
    }

    QJsonArray list;
    for (const BuildConfiguration &c : m_configs) list.append(c.toJson());
    const QJsonObject root{
        {QStringLiteral("version"), 1},
        {QStringLiteral("kind"), m_kind},
        {QStringLiteral("active"), active().name},
        {QStringLiteral("configurations"), list},
    };

    QSaveFile f(settingsFile());
    if (!f.open(QIODevice::WriteOnly)
        || f.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0
        || !f.commit()) {
        if (error) *error = tr("Cannot write %1: %2").arg(settingsFile(), f.errorString());
        return false;
    }
    return true;
}

QString BuildConfigurations::buildDirectory(const BuildConfiguration &config) const
{
    QString dir = config.buildDir.isEmpty() ? QStringLiteral(".") : config.buildDir;
    dir.replace(QLatin1String("${ProjectDir}"),  m_dir)
       .replace(QLatin1String("${ProjectName}"), m_name)
       .replace(QLatin1String("${Config}"),      config.name)
       .replace(QLatin1String("${KayteTools}"),  toolsDir());
    if (dir.startsWith(QLatin1String("~/")))
        dir = QDir::homePath() + dir.mid(1);
    return QDir::cleanPath(QDir(m_dir.isEmpty() ? QDir::currentPath() : m_dir).absoluteFilePath(dir));
}

QString BuildConfigurations::expand(const QString &command, const BuildConfiguration &config) const
{
    QString s = command;
    s.replace(QLatin1String("${FileBaseName}"), QFileInfo(m_currentFile).completeBaseName())
     .replace(QLatin1String("${File}"),        m_currentFile)
     .replace(QLatin1String("${BuildDir}"),    buildDirectory(config))
     .replace(QLatin1String("${ProjectDir}"),  m_dir)
     .replace(QLatin1String("${ProjectName}"), m_name)
     .replace(QLatin1String("${Config}"),      config.name)
     .replace(QLatin1String("${KayteTools}"),  toolsDir());
    return s;
}
