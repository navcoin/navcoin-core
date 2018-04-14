// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_UTILTIME_H
#define NAVCOIN_UTILTIME_H

#include <stdint.h>
#include <string>
#include <vector>

#define MINIMUM_NTP_MEASURE 3

int64_t GetTime();
int64_t GetTimeNow();
int64_t GetSteadyTime();
int64_t GetTimeMillis();
int64_t GetTimeMicros();
int64_t GetLogTimeMicros();
int64_t GetNtpTimeOffset();
void SetMockTime(int64_t nMockTimeIn);
void SetNtpTimeOffset(uint64_t nTimeOffsetIn);
void MilliSleep(int64_t n);

std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime);

static std::vector<std::string> vDefaultNtpServers = {"de.pool.ntp.org",
                  "0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org",
                  "0.de.pool.ntp.org", "1.de.pool.ntp.org", "2.de.pool.ntp.org",
                  "3.pool.ntp.org", "pool.ntp.org", "time.google.com",
                  "ntp0.fau.de", "ntps1-0.uni-erlangen.de", "ntps1-0.cs.tu-berlin.de",
                  "ptbtime1.ptb.de", "rustime01.rus.uni-stuttgart.de" };

#endif // NAVCOIN_UTILTIME_H
