// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTHREAD_H
#define TORTHREAD_H

#include <stdint.h>
#include <atomic>
#include <deque>
#include <thread>
#include <memory>
#include <string>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <util.h>
#include <scheduler.h>

void TorThreadInit();
void TorThreadStop();
void TorThreadStart();
void TorThreadInterrupt();

void TorRun();

#endif // TORTHREAD_H
