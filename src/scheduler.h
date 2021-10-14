// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_SCHEDULER_H
#define NAVCOIN_SCHEDULER_H

#include <condition_variable>
#include <functional>
#include <list>
#include <map>
#include <thread>

#include <sync.h>

/**
 * Simple class for background tasks that should be run
 * periodically or once "after a while"
 *
 * Usage:
 *
 * CScheduler* s = new CScheduler();
 * s->scheduleFromNow(doSomething, std::chrono::milliseconds{11}); // Assuming a: void doSomething() { }
 * s->scheduleFromNow([=] { this->func(argument); }, std::chrono::milliseconds{3});
 * std::thread* t = new std::thread([&] { s->serviceQueue(); });
 *
 * ... then at program shutdown, make sure to call stop() to clean up the thread(s) running serviceQueue:
 * s->stop();
 * t->join();
 * delete t;
 * delete s; // Must be done after thread is interrupted/joined.
 */

class CScheduler
{
public:
    CScheduler();
    ~CScheduler();

    typedef std::function<void(void)> Function;

    // Call func at/after time t
    void schedule(Function f, std::chrono::system_clock::time_point t);

    // Convenience method: call f once deltaSeconds from now
    void scheduleFromNow(Function f, int64_t deltaSeconds);

    /**
     * Repeat f until the scheduler is stopped. First run is after delta has passed once.
     *
     * The timing is not exact: Every time f is finished, it is rescheduled to run again after delta. If you need more
     * accurate scheduling, don't use this method.
     */
    void scheduleEvery(Function f, int64_t deltaSeconds);

    // To keep things as simple as possible, there is no unschedule.

    /**
     * Services the queue 'forever'. Should be run in a thread.
     */
    void serviceQueue();

    // Tell any threads running serviceQueue to stop as soon as they're
    // done servicing whatever task they're currently servicing (drain=false)
    // or when there is no work left to be done (drain=true)
    void stop(bool drain=false);

    // Returns number of tasks waiting to be serviced,
    // and first and last task times
    size_t getQueueInfo(std::chrono::system_clock::time_point &first,
                        std::chrono::system_clock::time_point &last) const;

private:
    std::multimap<std::chrono::system_clock::time_point, Function> taskQueue;
    std::condition_variable newTaskScheduled;
    mutable std::mutex newTaskMutex;
    int nThreadsServicingQueue;
    bool stopRequested;
    bool stopWhenEmpty;
    bool shouldStop() { return stopRequested || (stopWhenEmpty && taskQueue.empty()); }
};

#endif
