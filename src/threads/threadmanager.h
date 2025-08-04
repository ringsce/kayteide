#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <QObject>
#include <QList>
#include <QThread> // If using QThread wrapper or concepts

// Forward declaration if PosixThread is a separate class
class PosixThread;

class ThreadManager : public QObject
{
    Q_OBJECT
public:
    explicit ThreadManager(QObject *parent = nullptr);
    ~ThreadManager();

    // Example methods for your thread manager
    void startNewComputation();
    void stopAllComputations();

    signals:
        void computationFinished(int result);
    void message(const QString &msg);

private slots:
    void handleThreadFinished();
    void handleThreadMessage(const QString &msg);

private:
    QList<PosixThread*> m_activeThreads; // Or QList<QThread*> if using QThread
};

#endif // THREADMANAGER_H