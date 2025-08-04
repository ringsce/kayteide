#ifndef POSIXTHREAD_H
#define POSIXTHREAD_H

#include <QThread>
#include <QDebug>
// You might include <pthread.h> if you are doing raw pthreads,
// but QThread abstracts much of that for you.

class PosixThread : public QThread
{
    Q_OBJECT
public:
    explicit PosixThread(QObject *parent = nullptr);
    ~PosixThread();

protected:
    void run() override; // This method contains the thread's main logic

    signals:
        void message(const QString &msg);
    // QThread already has a finished() signal
};

#endif // POSIXTHREAD_H