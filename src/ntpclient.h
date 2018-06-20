// Copyright (c) 2018 alex v
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NTPCLIENT_H
#define NTPCLIENT_H

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <string>
#include <iostream>

#define DEFAULT_NTP_TIMEOUT 5

using namespace std;

class CNtpClient
{
    string sHostName;
  public:
    CNtpClient(string server) : sHostName(server) { }
    bool getTimestamp(uint64_t&);
};

bool NtpClockSync();

#endif // NTPCLIENT_H
