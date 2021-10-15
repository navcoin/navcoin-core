// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Functionality for communicating with Tor.
 */
#ifndef NAVCOIN_TORCONTROL_H
#define NAVCOIN_TORCONTROL_H

#include <scheduler.h>

extern const std::string DEFAULT_TOR_CONTROL;
static const bool DEFAULT_LISTEN_ONION = true;

class AggregationSession;

typedef std::function<void(std::string)> hidden_service_cb;

class TorControlThread
{
public:
    TorControlThread(int listen_in = 0, hidden_service_cb ready_cb_in = 0): listen(listen_in), ready_cb(ready_cb_in) {}

    void Run();
    void Start();
    void Interrupt();
    void Stop();
private:
    struct event_base *base;
    std::thread torControlThread;
    int listen;
    hidden_service_cb ready_cb;
};

#endif /* NAVCOIN_TORCONTROL_H */
