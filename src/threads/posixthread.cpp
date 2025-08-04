#include "posixthread.h"

PosixThread::PosixThread(QObject *parent) : QThread(parent)
{
    qDebug() << "PosixThread created.";
}

PosixThread::~PosixThread()
{
    qDebug() << "PosixThread destroyed.";
}

void PosixThread::run()
{
    // This is the code that will run in the separate thread
    qDebug() << "PosixThread running in thread: " << QThread::currentThreadId();
    emit message("Thread started computation...");

    // Simulate some work
    for (int i = 0; i < 5; ++i) {
        if (isInterruptionRequested()) {
            emit message("Thread interruption requested.");
            break;
        }
        qDebug() << "Thread working... " << i;
        QThread::sleep(1); // Simulate time-consuming operation
    }

    emit message("Thread finished computation.");
    qDebug() << "PosixThread finished.";
}