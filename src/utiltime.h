// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_UTILTIME_H
#define NAVCOIN_UTILTIME_H

#include <stdint.h>
#include <string>

#define MINIMUM_NTP_MEASURE 3

int64_t GetTime();
int64_t GetTimeMillis();
int64_t GetTimeMicros();
int64_t GetLogTimeMicros();
void SetMockTime(int64_t nMockTimeIn);
void MilliSleep(int64_t n);

std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime);

static int64_t nNtpTimeOffset = 0;
static std::vector<std::string> vDefaultNtpServers = {"0.pool.ntp.org", "1.pool.ntp.org",
                  "2.pool.ntp.org", "3.pool.ntp.org", "time-a-wwv.nist.gov",
                  "utcnist.colorado.edu", "time.google.com" };

#endif // NAVCOIN_UTILTIME_H
