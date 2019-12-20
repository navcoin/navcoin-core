// Copyright (c) 2018 alex v
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <ntpclient.h>
#include <random.h>
#include <timedata.h>
#include <util.h>

#include <sstream>
#include <iomanip>

using namespace boost;
using namespace boost::asio;

bool CNtpClient::getTimestamp(uint64_t &timeRecv)
{

    io_service io_service;

    LogPrint("ntp", "[NTP] Opening socket to NTP server %s.\n", sHostName);
    try
    {

        ip::udp::resolver resolver(io_service);
        ip::udp::resolver::query query(boost::asio::ip::udp::v4(), sHostName, "ntp");

        ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

        ip::udp::socket socket(io_service);
        socket.open(ip::udp::v4());

        boost::array<unsigned char, 48> sendBuf = {{010,0,0,0,0,0,0,0,0}};

        socket.send_to(boost::asio::buffer(sendBuf), receiver_endpoint);

        boost::array<uint32_t, 1024> recvBuf;
        ip::udp::endpoint sender_endpoint;

        try
        {

            fd_set fileDescriptorSet;
            struct timeval timeStruct;

            timeStruct.tv_sec = GetArg("-ntptimeout", DEFAULT_NTP_TIMEOUT);
            timeStruct.tv_usec = 0;
            FD_ZERO(&fileDescriptorSet);

            int nativeSocket = socket.native_handle();

            FD_SET(nativeSocket,&fileDescriptorSet);

            select(nativeSocket+1,&fileDescriptorSet,NULL,NULL,&timeStruct);

            if(!FD_ISSET(nativeSocket,&fileDescriptorSet))
            {

                LogPrint("ntp", "[NTP] Could not read socket from NTP server %s (Read timeout)\n", sHostName);

            }
            else
            {

                socket.receive_from(boost::asio::buffer(recvBuf), sender_endpoint);

                timeRecv = ntohl((uint32_t)recvBuf[8]);

                if(timeRecv > 2208988800U) // Sanity check
                {

                    timeRecv-= 2208988800U;  // Substract 01/01/1970 == 2208988800U
                    LogPrint("ntp", "[NTP] Received timestamp: %ll\n", (uint64_t)timeRecv);

                    return true;

                }
                else
                {

                    LogPrintf("[NTP] Received wrong clock from NTP server %s (bad timestamp format)\n", sHostName);

                }

            }

        }
        catch (std::exception& e)
        {

             LogPrintf("[NTP] Could not read clock from NTP server %s (%s)\n", sHostName, e.what());

        }

    }
    catch (std::exception& e)
    {

        LogPrintf("[NTP] Could not open socket to NTP server %s (%s)\n", sHostName, e.what());

    }

    return false;
}

bool NtpClockSync()
{
    LogPrintf("[NTP] Starting clock sync...\n");

    std::vector<std::string> vNtpServers;
    std::vector<int64_t> vResults;

    if (mapMultiArgs["-ntpserver"].size() > 0)
    {
        vNtpServers = mapMultiArgs["-ntpserver"];
    }
    else
    {
        vNtpServers = vDefaultNtpServers;
    }

    string sReport = "";
    string sPrevServer = "";
    int64_t nPrevMeasure = -1;

    random_shuffle(vNtpServers.begin(), vNtpServers.end(), GetRandInt);

    unsigned int nMeasureCount = 0;

    for(unsigned int i = 0; i < vNtpServers.size(); i++)
    {
        string s = vNtpServers[i];
        CNtpClient ntpClient(s);
        uint64_t nTimestamp = 0;

        if (ShutdownRequested())
            return false;

        if(ntpClient.getTimestamp(nTimestamp))
        {
            int64_t nClockDrift = GetTimeNow() - nTimestamp;
            nMeasureCount++;

            // We push if its the first entry
            if(nMeasureCount == 1)
            {
                vResults.push_back(nClockDrift);

                string sign = "";
                if(nClockDrift > 0)
                    sign = "+";

                sReport += s + "[" + sign + to_string(nClockDrift) + "sec.] ";
            }
            // or if n.measure is odd, including prev measure too
            else if (nMeasureCount % 2 == 1)
            {
                vResults.push_back(nClockDrift);

                string sign = "";
                if(nClockDrift > 0)
                    sign = "+";

                sReport += s + "[" + sign + to_string(nClockDrift) + "sec.] ";

                vResults.push_back(nPrevMeasure);

                sign = "";
                if(nPrevMeasure > 0)
                    sign = "+";

                sReport += sPrevServer + "[" + sign + to_string(nPrevMeasure) + "sec.] ";
            }
            else
            {
                nPrevMeasure = nClockDrift;
                sPrevServer = s;
            }

            if (vResults.size() >= 5)
                break;
        }
    }

    assert(vResults.size() % 2 == 1 || vResults.size() == 0);

    unsigned int nMin = GetArg("-ntpminmeasures", MINIMUM_NTP_MEASURE);

    if (vResults.size() >= nMin)
    {
        LogPrintf("[NTP] Measured: %s\n", sReport);

        static CMedianFilter<int64_t> vNtpTimeOffsets(vResults.size(), 0);

        for(unsigned int i = 0; i < vResults.size(); i++)
        {
            vNtpTimeOffsets.input(vResults[i]);
        }

        SetNtpTimeOffset(vNtpTimeOffsets.median());

        LogPrintf("[NTP] Calculated offset from median: %d\n", GetNtpTimeOffset());
        return true;
    }
    else
    {
        return false;
    }
}
