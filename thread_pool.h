#ifndef THREAD_POOL_H
#define THREAD_POOL_H
// *****************************************************************************
// thread_pool.h                                                   Tao3D project
// *****************************************************************************
//
// File description:
//
//    Execute tasks in separate thread(s).
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2014-2015,2019, Christophe de Dinechin <christophe@dinechin.org>
// (C) 2012, Jérôme Forissier <jerome@taodyne.com>
// *****************************************************************************
// This file is part of Tao3D
//
// Tao3D is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tao3D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Tao3D, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include <QList>
#include <QMutex>
#include <QRunnable>
#include <QThread>
#include <QWaitCondition>
#include "base.h"

class Runnable;
class Thread;

class ThreadPool
// ----------------------------------------------------------------------------
//    Manage tasks and threads
// ----------------------------------------------------------------------------
{
    friend class Runnable;
    friend class Thread;

public:
    ThreadPool(int maxThreads = 1)
        : isExiting(false), maxThreads(maxThreads), idleThreads(0) {}
    virtual ~ThreadPool() { stopAll(); }

public:
    void start(Runnable * runnable);

    void stopAll()
    {
        QMutexLocker locker(&mutex);
        runQueue.empty();
        isExiting = true;
        while (!threads.isEmpty())
        {
            runnableReady.wakeAll();
            noActiveThread.wait(&mutex);
        }
        isExiting = false;
    }

protected:
    void startThreadNolock(Runnable * runnable);

protected:
    typedef QList<Runnable *> run_queue;

    QMutex             mutex;
    run_queue          runQueue;
    QList<Thread *>    threads;
    QWaitCondition     runnableReady, noActiveThread;
    bool               isExiting;
    int                maxThreads, idleThreads;
};


class Runnable : public QRunnable
// ----------------------------------------------------------------------------
//    Base class for tasks run by ThreadPool
// ----------------------------------------------------------------------------
{
    friend class Thread;
    friend class ThreadPool;

public:
    Runnable() : QRunnable(), running(false), isInterrupted(false), pool(0) {}
    ~Runnable() { interrupt(); }

public:
    virtual void run() = 0;

    bool interrupted() { return isInterrupted; }

    void interrupt()
    {
        QMutexLocker locker(&mutex);
        if (pool)
        {
            QMutexLocker poolLocker(&pool->mutex);
            if (pool->runQueue.removeOne(this))
                return;
        }
        if (running)
        {
            isInterrupted = true;
            notRunning.wait(&mutex);
            isInterrupted = false;
            running = false;
        }
     }

protected:
    void runInternal()
    {
        QMutexLocker locker(&mutex);
        if (interrupted())
            return notRunning.wakeAll();
        running = true;
        locker.unlock();
        run();
        locker.relock();
        running = false;
        if (interrupted())
            notRunning.wakeAll();
    }

protected:
    bool            running, isInterrupted;
    QMutex          mutex;
    QWaitCondition  notRunning;
    ThreadPool *    pool;
};


class Thread : public QThread
// ----------------------------------------------------------------------------
//    Used by ThreadPool to execute Runnable objects
// ----------------------------------------------------------------------------
{
public:
    Thread(ThreadPool * pool) : runnable(0), pool(pool) {}
    ~Thread() {}

public:
    void run()
    {
        QMutexLocker locker(&pool->mutex);
        Runnable *r = runnable;
        runnable = NULL;
        for (;;)
        {
            while (r)
            {
                XL_ASSERT(pool->idleThreads > 0);
                pool->idleThreads--;
                locker.unlock();
                r->runInternal();
                locker.relock();
                pool->idleThreads++;
                if (pool->isExiting)
                    return removeFromPool();
                r = tryTakeFirstFromRunQueue();
            }
            do
            {
                pool->runnableReady.wait(&pool->mutex);
                if (pool->isExiting)
                    return removeFromPool();
                r = tryTakeFirstFromRunQueue();
            } while (!r);
        }
    }

public:
    Runnable * runnable;

protected:
    void removeFromPool()
    {
        pool->idleThreads--;
        pool->threads.removeOne(this);
        if (pool->threads.isEmpty())
            pool->noActiveThread.wakeOne();
    }

    Runnable * tryTakeFirstFromRunQueue()
    {
        ThreadPool::run_queue &q = pool->runQueue;
        Runnable * r = !q.isEmpty() ? q.takeFirst() : NULL;
        return r;
    }

protected:
    ThreadPool *  pool;
};


inline void ThreadPool::startThreadNolock(Runnable * runnable)
{
    Thread * thread = new Thread(this);
    threads.append(thread);
    idleThreads++;

    thread->runnable = runnable;
    runnable->running = true;
    thread->start();
}


inline void ThreadPool::start(Runnable *runnable)
{
    QMutexLocker locker(&mutex);
    QMutexLocker rlocker(&runnable->mutex);

    if (runQueue.contains(runnable) || runnable->running)
        return;

    if (idleThreads == 0 && threads.size() < maxThreads)
    {
        startThreadNolock(runnable);
    }
    else
    {
        runQueue.append(runnable);
        if (idleThreads)
            runnableReady.wakeOne();
    }
}

#endif // THREAD_POOL_H
