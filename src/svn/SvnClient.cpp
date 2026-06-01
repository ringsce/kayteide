// SvnClient.cpp – KaytEIDE SVN client implementation
// Drives the `svn` CLI via QProcess on a background thread.
// C++17, Qt 6

#include "SvnClient.h"

#include <QProcess>
#include <QXmlStreamReader>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QDebug>

namespace Kayte::Svn {

// ────────────────────────────────────────────────────────────────────────────
// Private implementation
// ────────────────────────────────────────────────────────────────────────────

struct SvnClientPrivate
{
    QString     svnExe { QStringLiteral("svn") };
    Credentials creds;

    struct RunResult {
        QString stdOut;
        QString stdErr;
        int     exitCode { -1 };
        bool    ok() const { return exitCode == 0; }
    };

    RunResult run(const QStringList &args) const
    {
        QProcess proc;
        proc.setProgram(svnExe);

        QStringList full = args;
        if (!creds.username.isEmpty())
            full << QStringLiteral("--username") << creds.username;
        if (!creds.password.isEmpty()) {
            full << QStringLiteral("--password") << creds.password;
            if (!creds.storePassword)
                full << QStringLiteral("--no-auth-cache");
        }
        full << QStringLiteral("--non-interactive");

        proc.setArguments(full);
        proc.start();
        proc.waitForFinished(60'000);

        return { QString::fromUtf8(proc.readAllStandardOutput()),
                 QString::fromUtf8(proc.readAllStandardError()),
                 proc.exitCode() };
    }

    // ── XML parsers ──────────────────────────────────────────────────────────

    QList<StatusEntry> parseStatusXml(const QString &xml) const
    {
        QList<StatusEntry> result;
        QXmlStreamReader rd(xml);
        StatusEntry current;
        bool inEntry = false;

        while (!rd.atEnd()) {
            rd.readNext();
            if (rd.isStartElement()) {
                const auto name = rd.name();
                if (name == QLatin1String("entry")) {
                    inEntry = true;
                    current = {};
                    current.path = rd.attributes().value(QLatin1String("path")).toString();
                } else if (inEntry && name == QLatin1String("wc-status")) {
                    auto a = rd.attributes();
                    current.textStatus   = statusKindFromString(a.value(QLatin1String("item")).toString());
                    current.propStatus   = statusKindFromString(a.value(QLatin1String("props")).toString());
                    current.versioned    = (current.textStatus != StatusKind::Unversioned);
                    current.switched     = (a.value(QLatin1String("switched")) == QLatin1String("true"));
                    current.treeConflict = (a.value(QLatin1String("tree-conflicted")) == QLatin1String("true"));
                    if (a.hasAttribute(QLatin1String("revision")))
                        current.revision = a.value(QLatin1String("revision")).toLongLong();
                } else if (inEntry && name == QLatin1String("author")) {
                    current.author = rd.readElementText();
                } else if (inEntry && name == QLatin1String("date")) {
                    current.lastChanged = QDateTime::fromString(rd.readElementText(), Qt::ISODateWithMs);
                }
            } else if (rd.isEndElement() && rd.name() == QLatin1String("entry")) {
                result.append(current);
                inEntry = false;
            }
        }
        return result;
    }

    QList<LogEntry> parseLogXml(const QString &xml) const
    {
        QList<LogEntry> result;
        QXmlStreamReader rd(xml);
        LogEntry entry;
        bool inEntry = false;

        while (!rd.atEnd()) {
            rd.readNext();
            if (rd.isStartElement()) {
                const auto name = rd.name();
                if (name == QLatin1String("logentry")) {
                    inEntry = true;
                    entry = {};
                    entry.revision = rd.attributes().value(QLatin1String("revision")).toLongLong();
                } else if (inEntry) {
                    if (name == QLatin1String("author"))
                        entry.author = rd.readElementText();
                    else if (name == QLatin1String("date"))
                        entry.date = QDateTime::fromString(rd.readElementText(), Qt::ISODateWithMs);
                    else if (name == QLatin1String("msg"))
                        entry.message = rd.readElementText();
                    else if (name == QLatin1String("path")) {
                        const QString kind = rd.attributes().value(QLatin1String("action")).toString();
                        entry.changedPaths.append(kind + QLatin1Char(' ') + rd.readElementText());
                    }
                }
            } else if (rd.isEndElement() && rd.name() == QLatin1String("logentry")) {
                result.append(entry);
                inEntry = false;
            }
        }
        return result;
    }

    WcInfo parseInfoXml(const QString &xml) const
    {
        WcInfo info;
        QXmlStreamReader rd(xml);
        while (!rd.atEnd()) {
            rd.readNext();
            if (!rd.isStartElement()) continue;
            const auto name = rd.name();
            if      (name == QLatin1String("url"))    info.url          = rd.readElementText();
            else if (name == QLatin1String("root"))   info.reposRootUrl = rd.readElementText();
            else if (name == QLatin1String("uuid"))   info.reposUuid    = rd.readElementText();
            else if (name == QLatin1String("entry")) {
                info.wcPath   = rd.attributes().value(QLatin1String("path")).toString();
                info.revision = rd.attributes().value(QLatin1String("revision")).toLongLong();
                info.nodeKind = rd.attributes().value(QLatin1String("kind")).toString();
            } else if (name == QLatin1String("commit")) {
                info.lastChanged = rd.attributes().value(QLatin1String("revision")).toLongLong();
            } else if (name == QLatin1String("author")) {
                info.lastAuthor = rd.readElementText();
            } else if (name == QLatin1String("date")) {
                info.lastDate = QDateTime::fromString(rd.readElementText(), Qt::ISODateWithMs);
            } else if (name == QLatin1String("schedule")) {
                info.schedule = rd.readElementText();
            }
        }
        return info;
    }

    QList<ListEntry> parseListXml(const QString &xml) const
    {
        QList<ListEntry> result;
        QXmlStreamReader rd(xml);
        ListEntry entry;
        bool inEntry = false;

        while (!rd.atEnd()) {
            rd.readNext();
            if (rd.isStartElement()) {
                const auto name = rd.name();
                if (name == QLatin1String("entry")) {
                    inEntry = true;
                    entry = {};
                    entry.isDir = (rd.attributes().value(QLatin1String("kind")) == QLatin1String("dir"));
                } else if (inEntry) {
                    if      (name == QLatin1String("name"))   entry.name   = rd.readElementText();
                    else if (name == QLatin1String("size"))   entry.size   = rd.readElementText().toLongLong();
                    else if (name == QLatin1String("author")) entry.author = rd.readElementText();
                    else if (name == QLatin1String("date"))   entry.date   = QDateTime::fromString(rd.readElementText(), Qt::ISODateWithMs);
                    else if (name == QLatin1String("commit"))
                        entry.revision = rd.attributes().value(QLatin1String("revision")).toLongLong();
                }
            } else if (rd.isEndElement() && rd.name() == QLatin1String("entry")) {
                result.append(entry);
                inEntry = false;
            }
        }
        return result;
    }

    QList<BlameLine> parseBlameXml(const QString &xml) const
    {
        QList<BlameLine> result;
        QXmlStreamReader rd(xml);
        BlameLine line;
        bool inEntry = false;

        while (!rd.atEnd()) {
            rd.readNext();
            if (rd.isStartElement()) {
                const auto name = rd.name();
                if (name == QLatin1String("entry")) {
                    inEntry = true;
                    line = {};
                } else if (inEntry) {
                    if      (name == QLatin1String("rev"))    line.revision = rd.readElementText().toLongLong();
                    else if (name == QLatin1String("author")) line.author   = rd.readElementText();
                    else if (name == QLatin1String("value"))  line.line     = rd.readElementText();
                }
            } else if (rd.isEndElement() && rd.name() == QLatin1String("entry")) {
                result.append(line);
                inEntry = false;
            }
        }
        return result;
    }

    static StatusKind statusKindFromString(const QString &s)
    {
        static const QHash<QString, StatusKind> map {
            { QStringLiteral("none"),        StatusKind::None        },
            { QStringLiteral("unversioned"), StatusKind::Unversioned },
            { QStringLiteral("normal"),      StatusKind::Normal      },
            { QStringLiteral("added"),       StatusKind::Added       },
            { QStringLiteral("missing"),     StatusKind::Missing     },
            { QStringLiteral("deleted"),     StatusKind::Deleted     },
            { QStringLiteral("replaced"),    StatusKind::Replaced    },
            { QStringLiteral("modified"),    StatusKind::Modified    },
            { QStringLiteral("merged"),      StatusKind::Merged      },
            { QStringLiteral("conflicted"),  StatusKind::Conflicted  },
            { QStringLiteral("ignored"),     StatusKind::Ignored     },
            { QStringLiteral("obstructed"),  StatusKind::Obstructed  },
            { QStringLiteral("external"),    StatusKind::External    },
            { QStringLiteral("incomplete"),  StatusKind::Incomplete  },
        };
        return map.value(s, StatusKind::None);
    }
};

// ────────────────────────────────────────────────────────────────────────────
// Internal helper: launch a fire-and-forget async task.
// The QFutureWatcher deletes itself once the task completes.
// Using a template function (not a macro) means the compiler handles commas
// inside lambda capture lists and bodies without any preprocessor confusion.
// ────────────────────────────────────────────────────────────────────────────

template<typename Func>
static void runAsync(QObject *parent, Func &&func)
{
    auto *watcher = new QFutureWatcher<void>(parent);
    QObject::connect(watcher, &QFutureWatcher<void>::finished,
                     watcher, &QObject::deleteLater);
    watcher->setFuture(QtConcurrent::run(std::forward<Func>(func)));
}

// ────────────────────────────────────────────────────────────────────────────
// SvnClient public API
// ────────────────────────────────────────────────────────────────────────────

SvnClient::SvnClient(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<SvnClientPrivate>())
{}

SvnClient::~SvnClient() = default;

void SvnClient::setCredentials(const Credentials &creds) { d->creds = creds; }
void SvnClient::setSvnExecutable(const QString &path)    { d->svnExe = path; }
QString SvnClient::svnExecutable() const                 { return d->svnExe; }

// ── status ────────────────────────────────────────────────────────────────

void SvnClient::status(const QString &wcPath, bool showUnversioned)
{
    runAsync(this, [this, wcPath, showUnversioned, priv = d.get()]() {
        QStringList args { QStringLiteral("status"), QStringLiteral("--xml") };
        if (showUnversioned)
            args << QStringLiteral("--no-ignore");
        args << wcPath;

        const auto res = priv->run(args);
        const auto entries = res.ok() ? priv->parseStatusXml(res.stdOut)
                                      : QList<StatusEntry>{};
        QMetaObject::invokeMethod(this, [this, entries, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("status"), res.stdErr);
            Q_EMIT statusReady(entries);
        }, Qt::QueuedConnection);
    });
}

// ── update ────────────────────────────────────────────────────────────────

void SvnClient::update(const QStringList &paths)
{
    runAsync(this, [this, paths, priv = d.get()]() {
        QStringList args { QStringLiteral("update"), QStringLiteral("--accept=postpone") };
        args << paths;
        const auto res = priv->run(args);

        qint64 rev = -1;
        static const QRegularExpression revRe(
            QStringLiteral("(?:Updated to|At) revision (\\d+)\\."));
        const auto m = revRe.match(res.stdOut);
        if (m.hasMatch()) rev = m.captured(1).toLongLong();

        QMetaObject::invokeMethod(this, [this, paths, rev, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("update"), res.stdErr);
            Q_EMIT updateFinished(paths, rev);
        }, Qt::QueuedConnection);
    });
}

// ── commit ────────────────────────────────────────────────────────────────

void SvnClient::commit(const QStringList &paths, const QString &message)
{
    runAsync(this, [this, paths, message, priv = d.get()]() {
        QStringList args { QStringLiteral("commit"), QStringLiteral("-m"), message };
        args << paths;
        const auto res = priv->run(args);

        CommitResult cr;
        cr.success = res.ok();
        if (res.ok()) {
            static const QRegularExpression revRe(
                QStringLiteral("Committed revision (\\d+)\\."));
            const auto m = revRe.match(res.stdOut);
            if (m.hasMatch()) cr.newRevision = m.captured(1).toLongLong();
        } else {
            cr.error = res.stdErr;
        }
        QMetaObject::invokeMethod(this, [this, cr]() {
            Q_EMIT commitFinished(cr);
        }, Qt::QueuedConnection);
    });
}

// ── add ───────────────────────────────────────────────────────────────────

void SvnClient::add(const QStringList &paths)
{
    runAsync(this, [this, paths, priv = d.get()]() {
        QStringList args { QStringLiteral("add"), QStringLiteral("--force") };
        args << paths;
        const auto res = priv->run(args);
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("add"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("add"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── revert ────────────────────────────────────────────────────────────────

void SvnClient::revert(const QStringList &paths)
{
    runAsync(this, [this, paths, priv = d.get()]() {
        QStringList args { QStringLiteral("revert"), QStringLiteral("--depth=infinity") };
        args << paths;
        const auto res = priv->run(args);
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("revert"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("revert"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── remove ────────────────────────────────────────────────────────────────

void SvnClient::remove(const QStringList &paths, bool keepLocal)
{
    runAsync(this, [this, paths, keepLocal, priv = d.get()]() {
        QStringList args { QStringLiteral("delete") };
        if (keepLocal) args << QStringLiteral("--keep-local");
        args << paths;
        const auto res = priv->run(args);
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("delete"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("delete"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── copy ─────────────────────────────────────────────────────────────────

void SvnClient::copy(const QString &src, const QString &dst)
{
    runAsync(this, [this, src, dst, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("copy"), src, dst });
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("copy"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("copy"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── move ─────────────────────────────────────────────────────────────────

void SvnClient::move(const QString &src, const QString &dst)
{
    runAsync(this, [this, src, dst, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("move"), src, dst });
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("move"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("move"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── mkdir ─────────────────────────────────────────────────────────────────

void SvnClient::mkdir(const QString &path, const QString &message)
{
    runAsync(this, [this, path, message, priv = d.get()]() {
        QStringList args { QStringLiteral("mkdir") };
        if (!message.isEmpty())
            args << QStringLiteral("-m") << message;
        args << path;
        const auto res = priv->run(args);
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("mkdir"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("mkdir"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── log ──────────────────────────────────────────────────────────────────

void SvnClient::log(const QString &path, int limit, qint64 startRev, qint64 endRev)
{
    runAsync(this, [this, path, limit, startRev, endRev, priv = d.get()]() {
        QStringList args {
            QStringLiteral("log"), QStringLiteral("--xml"),
            QStringLiteral("--verbose"),
            QStringLiteral("--limit"), QString::number(limit)
        };
        if (startRev >= 0 && endRev >= 0)
            args << QStringLiteral("-r")
                 << (QString::number(startRev) + QLatin1Char(':') + QString::number(endRev));
        args << path;

        const auto res = priv->run(args);
        const auto entries = res.ok() ? priv->parseLogXml(res.stdOut) : QList<LogEntry>{};
        QMetaObject::invokeMethod(this, [this, entries, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("log"), res.stdErr);
            Q_EMIT logReady(entries);
        }, Qt::QueuedConnection);
    });
}

// ── diff ─────────────────────────────────────────────────────────────────

void SvnClient::diff(const QString &path, qint64 rev1, qint64 rev2)
{
    runAsync(this, [this, path, rev1, rev2, priv = d.get()]() {
        QStringList args { QStringLiteral("diff") };
        if (rev1 >= 0 && rev2 >= 0)
            args << QStringLiteral("-r")
                 << (QString::number(rev1) + QLatin1Char(':') + QString::number(rev2));
        args << path;
        const auto res = priv->run(args);
        const DiffResult dr { path, res.stdOut };
        QMetaObject::invokeMethod(this, [this, dr, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("diff"), res.stdErr);
            Q_EMIT diffReady(dr);
        }, Qt::QueuedConnection);
    });
}

// ── blame ─────────────────────────────────────────────────────────────────

void SvnClient::blame(const QString &path, qint64 startRev, qint64 endRev)
{
    runAsync(this, [this, path, startRev, endRev, priv = d.get()]() {
        const QString revRange = QString::number(startRev) + QLatin1Char(':')
                               + (endRev >= 0 ? QString::number(endRev)
                                              : QStringLiteral("HEAD"));
        const auto res = priv->run({
            QStringLiteral("blame"), QStringLiteral("--xml"),
            QStringLiteral("-r"), revRange, path
        });
        const auto lines = res.ok() ? priv->parseBlameXml(res.stdOut) : QList<BlameLine>{};
        QMetaObject::invokeMethod(this, [this, lines, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("blame"), res.stdErr);
            Q_EMIT blameReady(lines);
        }, Qt::QueuedConnection);
    });
}

// ── info ─────────────────────────────────────────────────────────────────

void SvnClient::info(const QString &path)
{
    runAsync(this, [this, path, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("info"), QStringLiteral("--xml"), path });
        const WcInfo wi = res.ok() ? priv->parseInfoXml(res.stdOut) : WcInfo{};
        QMetaObject::invokeMethod(this, [this, wi, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("info"), res.stdErr);
            Q_EMIT infoReady(wi);
        }, Qt::QueuedConnection);
    });
}

// ── list ─────────────────────────────────────────────────────────────────

void SvnClient::list(const QString &url, bool recursive)
{
    runAsync(this, [this, url, recursive, priv = d.get()]() {
        QStringList args { QStringLiteral("list"), QStringLiteral("--xml") };
        if (recursive)
            args << QStringLiteral("--depth=infinity");
        args << url;
        const auto res = priv->run(args);
        const auto entries = res.ok() ? priv->parseListXml(res.stdOut) : QList<ListEntry>{};
        QMetaObject::invokeMethod(this, [this, entries, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("list"), res.stdErr);
            Q_EMIT listReady(entries);
        }, Qt::QueuedConnection);
    });
}

// ── checkout ──────────────────────────────────────────────────────────────

void SvnClient::checkout(const QString &url, const QString &localDir, qint64 rev)
{
    runAsync(this, [this, url, localDir, rev, priv = d.get()]() {
        QStringList args { QStringLiteral("checkout") };
        if (rev >= 0)
            args << QStringLiteral("-r") << QString::number(rev);
        args << url << localDir;

        QProcess proc;
        proc.setProgram(priv->svnExe);
        proc.setArguments(args);
        proc.start();
        const bool ok = proc.waitForFinished(600'000);
        const QString err = QString::fromUtf8(proc.readAllStandardError());
        const bool success = ok && proc.exitCode() == 0;

        QMetaObject::invokeMethod(this, [this, success, err]() {
            if (!success)
                Q_EMIT errorOccurred(QStringLiteral("checkout"), err);
            Q_EMIT checkoutFinished(success, err);
        }, Qt::QueuedConnection);
    });
}

// ── exportWc ─────────────────────────────────────────────────────────────

void SvnClient::exportWc(const QString &src, const QString &dst)
{
    runAsync(this, [this, src, dst, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("export"), QStringLiteral("--force"), src, dst });
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("export"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("export"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── cleanup ──────────────────────────────────────────────────────────────

void SvnClient::cleanup(const QString &path)
{
    runAsync(this, [this, path, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("cleanup"), path });
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("cleanup"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("cleanup"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── resolve ──────────────────────────────────────────────────────────────

void SvnClient::resolve(const QStringList &paths, ResolveChoice choice)
{
    runAsync(this, [this, paths, choice, priv = d.get()]() {
        static const QMap<ResolveChoice, QString> cm {
            { ResolveChoice::Postpone,   QStringLiteral("postpone")        },
            { ResolveChoice::Mine,       QStringLiteral("mine-conflict")   },
            { ResolveChoice::Theirs,     QStringLiteral("theirs-conflict") },
            { ResolveChoice::MineFull,   QStringLiteral("mine-full")       },
            { ResolveChoice::TheirsFull, QStringLiteral("theirs-full")     },
            { ResolveChoice::Merged,     QStringLiteral("working")         },
        };
        QStringList args { QStringLiteral("resolve"), QStringLiteral("--accept"), cm.value(choice) };
        args << paths;
        const auto res = priv->run(args);
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("resolve"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("resolve"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── merge ─────────────────────────────────────────────────────────────────

void SvnClient::merge(const QString &src, const QString &target, qint64 rev1, qint64 rev2)
{
    runAsync(this, [this, src, target, rev1, rev2, priv = d.get()]() {
        QStringList args { QStringLiteral("merge") };
        if (rev1 >= 0 && rev2 >= 0)
            args << QStringLiteral("-r")
                 << (QString::number(rev1) + QLatin1Char(':') + QString::number(rev2));
        args << src << target;
        const auto res = priv->run(args);
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("merge"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("merge"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── switchUrl ─────────────────────────────────────────────────────────────

void SvnClient::switchUrl(const QString &path, const QString &newUrl)
{
    runAsync(this, [this, path, newUrl, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("switch"), newUrl, path });
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("switch"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("switch"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── propSet ──────────────────────────────────────────────────────────────

void SvnClient::propSet(const QString &path, const QString &propName, const QString &value)
{
    runAsync(this, [this, path, propName, value, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("propset"), propName, value, path });
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("propset"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("propset"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── propGet ──────────────────────────────────────────────────────────────

void SvnClient::propGet(const QString &path, const QString &propName)
{
    runAsync(this, [this, path, propName, priv = d.get()]() {
        const auto res = priv->run({ QStringLiteral("propget"), propName, path });
        QMetaObject::invokeMethod(this, [this, res]() {
            if (!res.ok())
                Q_EMIT errorOccurred(QStringLiteral("propget"), res.stdErr);
            Q_EMIT operationFinished(QStringLiteral("propget"), res.ok(), res.stdOut);
        }, Qt::QueuedConnection);
    });
}

// ── Synchronous helpers ──────────────────────────────────────────────────

WcInfo SvnClient::infoSync(const QString &path) const
{
    const auto res = d->run({ QStringLiteral("info"), QStringLiteral("--xml"), path });
    return res.ok() ? d->parseInfoXml(res.stdOut) : WcInfo{};
}

QList<StatusEntry> SvnClient::statusSync(const QString &path, bool showUnversioned) const
{
    QStringList args { QStringLiteral("status"), QStringLiteral("--xml") };
    if (showUnversioned) args << QStringLiteral("--no-ignore");
    args << path;
    const auto res = d->run(args);
    return res.ok() ? d->parseStatusXml(res.stdOut) : QList<StatusEntry>{};
}

} // namespace Kayte::Svn
