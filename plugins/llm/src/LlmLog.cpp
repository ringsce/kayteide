#include "LlmLog.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QStandardPaths>
#include <QTextStream>

namespace Kayte::Llm::LlmLog {

namespace {
constexpr qint64 kMaxBytes = 2 * 1024 * 1024;   // rotate to llm.log.1 beyond 2 MB
QMutex           g_mutex;
}

QString path()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/llm.log");
}

void write(const QString &category, const QString &message)
{
    QMutexLocker lock(&g_mutex);
    const QString file = path();
    if (QFileInfo(file).size() > kMaxBytes) {
        QFile::remove(file + QStringLiteral(".1"));
        QFile::rename(file, file + QStringLiteral(".1"));
    }
    QFile f(file);
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    QTextStream out(&f);
    out << QDateTime::currentDateTime().toString(Qt::ISODate) << "  ["
        << category << "]  " << QString(message).replace(QLatin1Char('\n'), QLatin1Char(' '))
        << '\n';
}

} // namespace Kayte::Llm::LlmLog
