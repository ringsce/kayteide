#ifndef GITCLIENTPLUGIN_H
#define GITCLIENTPLUGIN_H

#include <QObject>
#include <QFuture>
#include "IGitClient.h"

// Your specific Git library includes (e.g., from libgit2)
#include <git2.h>

/**
 * @class GitClientPlugin
 * @brief A concrete Git client implementation using libgit2.
 */
class GitClientPlugin : public QObject, public IGitClient
{
    Q_OBJECT
    Q_INTERFACES(IGitClient)
    Q_PLUGIN_METADATA(IID "KayteIDE.IGitClient/1.0" FILE "GitClientPlugin.json")

public:
    explicit GitClientPlugin(QObject *parent = nullptr);
    ~GitClientPlugin() override;

    QFuture<void> clone(const QString& url, const QString& localPath) override;
    bool isRepository(const QString& path) const override;
    QStringList getFilesInRepo(const QString& localPath) override;
};

#endif // GITCLIENTPLUGIN_H