#include "CodeIndex.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QtConcurrent/QtConcurrentMap>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <cmath>

namespace Kayte::Llm {

namespace {

constexpr int    kChunkLines   = 60;
constexpr int    kChunkOverlap = 10;
constexpr qint64 kMaxFileBytes = 512 * 1024;
constexpr int    kMaxFiles     = 5000;
constexpr int    kBatchFiles   = 64;     // files tokenized in parallel per batch

bool isSkippedDir(const QString &name)
{
    static const QSet<QString> skip = {
        QStringLiteral(".git"), QStringLiteral(".svn"), QStringLiteral(".idea"),
        QStringLiteral(".vscode"), QStringLiteral("node_modules"),
        QStringLiteral("CMakeFiles"), QStringLiteral("_deps"),
        QStringLiteral("dist"), QStringLiteral("out"),
    };
    return skip.contains(name) || name.startsWith(QLatin1String("build")) ||
           name.startsWith(QLatin1String("cmake-build")) || name.endsWith(QLatin1String(".app"));
}

// Identifiers plus their camelCase / snake_case parts, lower-cased.
QStringList tokenize(const QString &text)
{
    static const QRegularExpression ident(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*"));
    static const QRegularExpression camel(QStringLiteral("[A-Z]?[a-z]+|[A-Z]+(?![a-z])|[0-9]+"));

    QStringList out;
    auto it = ident.globalMatch(text);
    while (it.hasNext()) {
        const QString word = it.next().captured();
        if (word.size() < 2) continue;
        out << word.toLower();
        auto parts = camel.globalMatch(word);
        int count = 0;
        QStringList pieces;
        while (parts.hasNext()) {
            const QString p = parts.next().captured().toLower();
            if (p.size() >= 3) pieces << p;
            ++count;
        }
        if (count > 1) out << pieces;
    }
    return out;
}

QStringList splitLines(const QString &absPath)
{
    QFile f(absPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
}

// Work item for one file, produced on a worker thread.
struct FileTerms
{
    struct Chunk { int startLine; int endLine; int length; QHash<QString, int> terms; };
    QString        rel;
    QString        language;
    QVector<Chunk> chunks;
};

FileTerms tokenizeFile(const QString &root, const QString &rel, const QString &language)
{
    FileTerms out { rel, language, {} };
    const QStringList lines = splitLines(root + QLatin1Char('/') + rel);
    for (int start = 0; start < lines.size(); start += kChunkLines - kChunkOverlap) {
        const int end = qMin<int>(start + kChunkLines, lines.size());
        const QString text = lines.mid(start, end - start).join(QLatin1Char('\n'));
        if (!text.trimmed().isEmpty()) {
            FileTerms::Chunk c { start + 1, end, 0, {} };
            // File names are strong signals: index the path too.
            const QStringList toks = tokenize(rel + QLatin1Char(' ') + text);
            for (const QString &t : toks) c.terms[t] += 1;
            c.length = toks.size();
            out.chunks << std::move(c);
        }
        if (end == lines.size()) break;
    }
    return out;
}

IndexData buildIndex(const QString &root)
{
    IndexData data;
    data.root = QDir(root).absolutePath();
    const QDir base(data.root);

    // 1. Walk the tree (pruning skipped directories) to list the files.
    QStringList files, languages;
    QStringList stack { data.root };
    while (!stack.isEmpty() && files.size() < kMaxFiles) {
        const QDir dir(stack.takeLast());
        const auto entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                               QDir::Name);
        for (const QFileInfo &fi : entries) {
            if (fi.isSymLink()) continue;
            if (fi.isDir()) {
                if (!isSkippedDir(fi.fileName())) stack << fi.absoluteFilePath();
                continue;
            }
            const QString lang = CodeIndex::languageFor(fi.fileName());
            if (lang.isEmpty() || fi.size() > kMaxFileBytes) continue;
            files << base.relativeFilePath(fi.absoluteFilePath());
            languages << lang;
            if (files.size() >= kMaxFiles) break;
        }
    }

    // 2. Tokenize in parallel, a batch at a time so temporary term tables for
    //    the whole project never sit in memory at once; 3. fold each batch
    //    into the shared vocabulary / postings.
    double totalLength = 0;
    for (int b = 0; b < files.size(); b += kBatchFiles) {
        QVector<int> batch;
        for (int i = b; i < qMin(b + kBatchFiles, int(files.size())); ++i) batch << i;

        const QList<FileTerms> results = QtConcurrent::blockingMapped<QList<FileTerms>>(
            batch, [&](int i) { return tokenizeFile(data.root, files.at(i), languages.at(i)); });

        for (const FileTerms &ft : results) {
            const int fileIdx = data.files.size();
            data.files << ft.rel;
            data.fileLanguages << ft.language;
            data.filesPerLanguage[ft.language] += 1;
            for (const FileTerms::Chunk &c : ft.chunks) {
                const int chunkIdx = data.chunks.size();
                data.chunks.append({ fileIdx, c.startLine, c.endLine, c.length });
                totalLength += c.length;
                for (auto it = c.terms.cbegin(); it != c.terms.cend(); ++it) {
                    auto v = data.vocabulary.constFind(it.key());
                    int id;
                    if (v == data.vocabulary.cend()) {
                        id = data.postings.size();
                        data.vocabulary.insert(it.key(), id);
                        data.postings.emplaceBack();
                    } else {
                        id = v.value();
                    }
                    data.postings[id].append({ chunkIdx, it.value() });
                }
            }
        }
    }

    for (auto &p : data.postings) p.squeeze();
    data.chunks.squeeze();
    data.avgLength = data.chunks.isEmpty() ? 0 : totalLength / data.chunks.size();
    return data;
}

} // namespace

CodeIndex::CodeIndex(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<IndexData>::finished, this, [this] {
        m_data = m_watcher.result();
        m_watcher.setFuture({});   // drop the future's copy of the index
        emit ready();
    });
}

CodeIndex::~CodeIndex()
{
    m_watcher.waitForFinished();   // a running build references nothing of ours
}

QString CodeIndex::languageFor(const QString &fileName)
{
    const QString lower  = fileName.toLower();
    if (lower == QLatin1String("cmakelists.txt")) return QStringLiteral("CMake");

    const QString suffix = QFileInfo(lower).suffix();
    static const QHash<QString, QString> map = {
        { "c", "C" },     { "h", "C" },
        { "cpp", "C++" }, { "cc", "C++" }, { "cxx", "C++" }, { "c++", "C++" },
        { "hpp", "C++" }, { "hh", "C++" }, { "hxx", "C++" }, { "ipp", "C++" },
        { "kayte", "Kayte" }, { "kyt", "Kayte" }, { "xproj", "Kayte" },
        { "vb", "VB" },   { "vbs", "VB" },  { "frm", "VB" },  { "cls", "VB" },
        { "bas", "BASIC" },
        { "cmake", "CMake" },
    };
    return map.value(suffix);
}

void CodeIndex::build(const QString &projectRoot)
{
    if (m_watcher.isRunning()) return;
    m_watcher.setFuture(QtConcurrent::run(buildIndex, projectRoot));
}

bool CodeIndex::isBuilding() const
{
    return m_watcher.isRunning();
}

QString CodeIndex::summary() const
{
    QStringList parts;
    for (auto it = m_data.filesPerLanguage.cbegin(); it != m_data.filesPerLanguage.cend(); ++it)
        parts << QStringLiteral("%1 %2").arg(it.key()).arg(it.value());
    return parts.join(QStringLiteral(" · "));
}

CodeChunk CodeIndex::materialize(int chunkIndex) const
{
    const IndexData::Chunk &c = m_data.chunks.at(chunkIndex);
    CodeChunk out;
    out.file      = m_data.files.at(c.file);
    out.language  = m_data.fileLanguages.at(c.file);
    out.startLine = c.startLine;
    out.endLine   = c.endLine;
    const QStringList lines = splitLines(m_data.root + QLatin1Char('/') + out.file);
    out.text = lines.mid(c.startLine - 1, c.endLine - c.startLine + 1).join(QLatin1Char('\n'));
    return out;
}

// Okapi BM25, walking only the postings of the query's terms.
QVector<CodeChunk> CodeIndex::search(const QString &query, int limit) const
{
    if (m_data.chunks.isEmpty()) return {};

    QSet<int> termIds;
    for (const QString &t : tokenize(query)) {
        const auto it = m_data.vocabulary.constFind(t);
        if (it != m_data.vocabulary.cend()) termIds.insert(it.value());
    }
    if (termIds.isEmpty()) return {};

    constexpr double k1 = 1.2, b = 0.75;
    const double N = m_data.chunks.size();
    const double avg = qMax(1.0, m_data.avgLength);

    QHash<int, double> scores;
    for (int id : std::as_const(termIds)) {
        const auto &list = m_data.postings.at(id);
        const double df  = list.size();
        const double idf = std::log(1.0 + (N - df + 0.5) / (df + 0.5));
        for (const auto &[chunk, tf] : list) {
            const double len = m_data.chunks.at(chunk).length;
            scores[chunk] += idf * (tf * (k1 + 1)) / (tf + k1 * (1 - b + b * len / avg));
        }
    }

    QVector<QPair<double, int>> ranked;
    ranked.reserve(scores.size());
    for (auto it = scores.cbegin(); it != scores.cend(); ++it) ranked.append({ it.value(), it.key() });
    const int n = qMin(limit, int(ranked.size()));
    std::partial_sort(ranked.begin(), ranked.begin() + n, ranked.end(),
                      [](const auto &a, const auto &b) { return a.first > b.first; });

    QVector<CodeChunk> out;
    for (int i = 0; i < n; ++i) out << materialize(ranked.at(i).second);
    return out;
}

QVector<CodeChunk> CodeIndex::chunksForFile(const QString &relativePath) const
{
    const int fileIdx = m_data.files.indexOf(relativePath);
    QVector<CodeChunk> out;
    if (fileIdx < 0) return out;
    const QStringList lines = splitLines(m_data.root + QLatin1Char('/') + relativePath);
    for (const IndexData::Chunk &c : m_data.chunks) {
        if (c.file != fileIdx) continue;
        out.append({ relativePath, m_data.fileLanguages.at(fileIdx), c.startLine, c.endLine,
                     lines.mid(c.startLine - 1, c.endLine - c.startLine + 1).join(QLatin1Char('\n')) });
    }
    return out;
}

} // namespace Kayte::Llm
