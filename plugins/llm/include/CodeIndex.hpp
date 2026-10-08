#pragma once

#include <QFutureWatcher>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVector>

namespace Kayte::Llm {

// A slice of a source file (~60 lines). Returned by searches; the text is read
// from disk on demand, the index itself never keeps file contents in memory.
struct CodeChunk
{
    QString file;       // path relative to the project root
    QString language;
    int     startLine { 1 };
    int     endLine   { 1 };
    QString text;
};

// Compact inverted index: each distinct identifier is stored once, with the
// list of chunks it occurs in. Memory grows with vocabulary, not file size.
struct IndexData
{
    struct Chunk { int file; int startLine; int endLine; int length; };
    using Posting = QPair<int, int>;               // (chunk index, term frequency)

    QString                    root;
    QStringList                files;             // relative paths
    QStringList                fileLanguages;     // parallel to files
    QVector<Chunk>             chunks;
    QHash<QString, int>        vocabulary;        // term → id
    QVector<QVector<Posting>>  postings;          // id → occurrences
    QMap<QString, int>         filesPerLanguage;
    double                     avgLength { 0 };
};

// "Learns" a project by reading its C, C++, Kayte, VB, BASIC and CMake files
// into a BM25 keyword index. Indexing runs on worker threads (one file per
// core at a time) and never on the GUI thread.
class CodeIndex : public QObject
{
    Q_OBJECT
public:
    explicit CodeIndex(QObject *parent = nullptr);
    ~CodeIndex() override;

    static QString languageFor(const QString &fileName);   // empty if unsupported

    void build(const QString &projectRoot);
    bool isBuilding() const;
    bool isReady() const { return !m_data.chunks.isEmpty(); }

    QString root() const       { return m_data.root; }
    int     fileCount() const  { return m_data.files.size(); }
    int     chunkCount() const { return m_data.chunks.size(); }
    QStringList files() const  { return m_data.files; }
    QString summary() const;   // "C++ 42 · CMake 3 · …"

    QVector<CodeChunk> search(const QString &query, int limit) const;
    QVector<CodeChunk> chunksForFile(const QString &relativePath) const;

signals:
    void ready();

private:
    CodeChunk materialize(int chunkIndex) const;

    IndexData                 m_data;
    QFutureWatcher<IndexData> m_watcher;
};

} // namespace Kayte::Llm
