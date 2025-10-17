#ifndef IGITCLIENT_H
#define IGITCLIENT_H

#include <QObject>
#include <QString>
#include <QFuture>

// Forward declaration of the GitRepository class to avoid a dependency
class GitRepository;

/**
 * @class IGitClient
 * @brief Abstract interface for a Git client plugin.
 *
 * This interface defines the core functionalities that any Git client plugin
 * must provide. It uses Qt's signal/slot mechanism for asynchronous operations.
 */
class IGitClient
{
public:
    virtual ~IGitClient() = default;

    /**
     * @brief Clones a Git repository from a given URL to a local path.
     * @param url The URL of the remote Git repository.
     * @param localPath The local directory where the repository should be cloned.
     * @return A QFuture object to monitor the progress and result of the clone operation.
     */
    virtual QFuture<void> clone(const QString& url, const QString& localPath) = 0;

    /**
     * @brief Checks if a given path is a valid Git repository.
     * @param path The path to check.
     * @return true if it's a Git repository, false otherwise.
     */
    virtual bool isRepository(const QString& path) const = 0;

    /**
     * @brief Gets a list of files in the working directory of a Git repository.
     * @param localPath The local path of the repository.
     * @return A list of file paths.
     */
    virtual QStringList getFilesInRepo(const QString& localPath) = 0;

    /**
     * @brief Registers the plugin with Qt's plugin system.
     *
     * This macro is essential for making the interface discoverable.
     */
    Q_PLUGIN_INTERFACE("KayteIDE.IGitClient/1.0")
};

#endif // IGITCLIENT_H