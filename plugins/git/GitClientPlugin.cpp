#include "GitClientPlugin.h"
#include <QtConcurrent/QtConcurrent>
#include <QDir>
#include <QFileInfo>
#include <QProcess> // Using QProcess as a fallback for simplicity

GitClientPlugin::GitClientPlugin(QObject *parent)
    : QObject(parent)
{
    // Initialize libgit2
    git_libgit2_init();
}

GitClientPlugin::~GitClientPlugin()
{
    // Shutdown libgit2
    git_libgit2_shutdown();
}

QFuture<void> GitClientPlugin::clone(const QString& url, const QString& localPath)
{
    // Perform the clone operation in a separate thread to avoid freezing the UI.
    return QtConcurrent::run([url, localPath]() {
        // Here you would use libgit2's API. For simplicity, we use QProcess.
        QProcess process;
        QStringList arguments;
        arguments << "clone" << url << localPath;
        process.start("git", arguments);
        process.waitForFinished(-1); // Wait indefinitely for the process to finish
        if (process.exitCode() != 0) {
            throw std::runtime_error(process.readAllStandardError().toStdString());
        }
    });
}

bool GitClientPlugin::isRepository(const QString& path) const
{
    // Check if the directory contains a .git folder.
    // This is a simplified check. A more robust implementation would use libgit2.
    QDir dir(path);
    return dir.exists(".git");
}

QStringList GitClientPlugin::getFilesInRepo(const QString& localPath)
{
    // This is a placeholder. A proper implementation would use libgit2's
    // tree traversal functions to list all tracked files.
    QProcess process;
    process.start("git", QStringList() << "ls-files" << "-c" << "--cached" << "--others" << "--exclude-standard");
    process.setWorkingDirectory(localPath);
    process.waitForFinished(-1);
    
    QStringList files;
    if (process.exitCode() == 0) {
        files = QString(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    }
    return files;
}