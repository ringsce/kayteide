#include "threadmanager.h"
#include "posixthread.h" // Include your specific thread implementation header

#include <QDebug>

ThreadManager::ThreadManager(QObject *parent) : QObject(parent)
{
    qDebug() << "ThreadManager created.";
}

ThreadManager::~ThreadManager()
{
    stopAllComputations();
    qDebug() << "ThreadManager destroyed.";
}

void ThreadManager::startNewComputation()
{
    qDebug() << "Starting new computation...";
    // Example: Create and start a PosixThread
    PosixThread *thread = new PosixThread(this); // Pass this as parent for proper memory management
    
    // Connect signals/slots for cleanup and communication
    connect(thread, &PosixThread::finished, this, &ThreadManager::handleThreadFinished);
    connect(thread, &PosixThread::message, this, &ThreadManager::handleThreadMessage);
    
    // Auto-delete the thread object when it finishes
    connect(thread, &PosixThread::finished, thread, &QObject::deleteLater);

    m_activeThreads.append(thread);
    thread->start(); // Assuming your PosixThread has a start method
}

void ThreadManager::stopAllComputations()
{
    qDebug() << "Stopping all computations...";
    for (PosixThread *thread : m_activeThreads) {
        if (thread->isRunning()) { // Assuming a method to check if running
            thread->requestInterruption(); // Request thread to stop gracefully
            thread->wait(5000); // Wait up to 5 seconds for it to finish
            if (thread->isRunning()) {
                qWarning() << "Thread did not stop gracefully, terminating.";
                thread->terminate(); // Force terminate if it doesn't stop
                thread->wait(); // Wait for it to actually terminate
            }
        }
    }
    m_activeThreads.clear();
}

void ThreadManager::handleThreadFinished()
{
    PosixThread *thread = qobject_cast<PosixThread*>(sender());
    if (thread) {
        m_activeThreads.removeOne(thread);
        qDebug() << "Thread finished. Active threads: " << m_activeThreads.count();
        emit computationFinished(0); // Example result
    }
}

void ThreadManager::handleThreadMessage(const QString &msg)
{
    qDebug() << "Message from thread: " << msg;
    emit message(msg);
}