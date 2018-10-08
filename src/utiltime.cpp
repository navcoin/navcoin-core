// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/navcoin-config.h"
#endif

#include "utiltime.h"

#include <chrono>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

using namespace std;

static int64_t nMockTime = 0; //!< For unit testing
static int64_t nNtpTimeOffset = 0;

int64_t GetTime()
{
    if (nMockTime) return nMockTime;

    time_t now = time(NULL);
    assert(now > 0);
    return now - GetNtpTimeOffset();
}

int64_t GetTimeNow()
{
    if (nMockTime) return nMockTime;

    time_t now = time(NULL);
    assert(now > 0);
    return now;
}

int64_t GetSteadyTime()
{
    auto now = std::chrono::steady_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    assert(millisecs.count() > 0);
    return millisecs.count();
}

void SetMockTime(int64_t nMockTimeIn)
{
    nMockTime = nMockTimeIn;
}

int64_t GetTimeMillis()
{
    int64_t now = (boost::posix_time::microsec_clock::universal_time() -
                   boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_milliseconds();
    assert(now > 0);
    return now - (GetNtpTimeOffset() * 1000);
}

int64_t GetTimeMicros()
{
    int64_t now = (boost::posix_time::microsec_clock::universal_time() -
                   boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_microseconds();
    assert(now > 0);
    return now;
}

/** Return a time useful for the debug log */
int64_t GetLogTimeMicros()
{
    if (nMockTime) return nMockTime*1000000;

    return GetTimeMicros();
}

void MilliSleep(int64_t n)
{

/**
 * Boost's sleep_for was uninterruptable when backed by nanosleep from 1.50
 * until fixed in 1.52. Use the deprecated sleep method for the broken case.
 * See: https://svn.boost.org/trac/boost/ticket/7238
 */
#if defined(HAVE_WORKING_BOOST_SLEEP_FOR)
    boost::this_thread::sleep_for(boost::chrono::milliseconds(n));
#elif defined(HAVE_WORKING_BOOST_SLEEP)
    boost::this_thread::sleep(boost::posix_time::milliseconds(n));
#else
//should never get here
#error missing boost sleep implementation
#endif
}

std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime)
{
    // std::locale takes ownership of the pointer
    std::locale loc(std::locale::classic(), new boost::posix_time::time_facet(pszFormat));
    std::stringstream ss;
    ss.imbue(loc);
    ss << boost::posix_time::from_time_t(nTime);
    return ss.str();
}

int64_t GetNtpTimeOffset()
{
    return nNtpTimeOffset;
}

void SetNtpTimeOffset(uint64_t nTimeOffsetIn)
{
    nNtpTimeOffset = nTimeOffsetIn;
}

std::string StringifySeconds(uint64_t n)
{
    using namespace std::chrono;
    std::chrono::seconds input_seconds{n};
    typedef duration<int, std::ratio<30 * 86400>> months;
    auto M = duration_cast<months>(input_seconds);
    input_seconds -= M;
    typedef duration<int, std::ratio<86400>> days;
    auto d = duration_cast<days>(input_seconds);
    input_seconds -= d;
    auto h = duration_cast<hours>(input_seconds);
    input_seconds -= h;
    auto m = duration_cast<minutes>(input_seconds);
    input_seconds -= m;
    auto s = duration_cast<seconds>(input_seconds);

    auto Mc = M.count();
    auto dc = d.count();
    auto hc = h.count();
    auto mc = m.count();
    auto sc = s.count();

    std::stringstream ss;
    ss.fill('0');
    if (Mc) {
        ss << M.count() << " months ";
    }

    if (Mc || dc) {
        ss << d.count() << " days ";
    }
    if (Mc || dc || hc) {
        if (Mc || dc) { ss << std::setw(2); }
        ss << h.count() << " hours ";
    }
    if (Mc || dc || hc || mc) {
        if (Mc || dc || hc) { ss << std::setw(2); }
        ss << m.count() << " minutes ";
    }
    if (Mc || dc || hc || mc || sc) {
        if (Mc || dc || hc || mc) { ss << std::setw(2); }
        ss << s.count() << " seconds";
    }

    return ss.str();
}
