// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_UTILTIME_H
#define NAVCOIN_UTILTIME_H

#include <stdint.h>
#include <string>
#include <vector>
#include <chrono>

#define MINIMUM_NTP_MEASURE 3
#define MAXIMUM_TIME_OFFSET 30

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

std::string StringifySeconds(uint64_t n);

static std::vector<std::string> vDefaultNtpServers = {"de.pool.ntp.org",
                  "jp.pool.ntp.org", "id.pool.ntp.org", "in.pool.ntp.org",
                  "0.de.pool.ntp.org", "1.de.pool.ntp.org", "2.de.pool.ntp.org",
                  "3.de.ntp.org", "cn.pool.ntp.org", "time.google.com",
                  "ntp0.fau.de", "ntps1-0.uni-erlangen.de", "ntps1-0.cs.tu-berlin.de",
                  "ptbtime1.ptb.de", "rustime01.rus.uni-stuttgart.de",
                  "time1.google.com", "time2.google.com", "time3.google.com", "time4.google.com",
                  "hora.rediris.es", "cuco.rediris.es", "hora.roa.es", "ntp.i2t.ehu.es",
                  "at.pool.ntp.org", "bg.pool.ntp.org", "ch.pool.ntp.org", "cz.pool.ntp.org",
                  "dk.pool.ntp.org", "fi.pool.ntp.org", "fr.pool.ntp.org", "hu.pool.ntp.org",
                  "nl.pool.ntp.org", "no.pool.ntp.org", "pl.pool.ntp.org", "ru.pool.ntp.org",
                  "uk.pool.ntp.org", "ca.pool.ntp.org", "us.pool.ntp.org", "au.pool.ntp.org",
                  "nz.pool.ntp.org", "br.pool.ntp.org"};

#endif // NAVCOIN_UTILTIME_H
