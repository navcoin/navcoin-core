// Copyright (c) 2018 alex v
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ntpclient.h"
#include "util.h"

using namespace boost;
using namespace boost::asio;

int64_t CNtpClient::getTimestamp()
{
    time_t timeRecv = -1;

    io_service io_service;

    LogPrint("ntp", "CNtpClient: Opening socket to NTP server %s.\n", sHostName);
    try
    {

        ip::udp::resolver resolver(io_service);
        ip::udp::resolver::query query(boost::asio::ip::udp::v4(), sHostName, "ntp");

        ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

        ip::udp::socket socket(io_service);
        socket.open(ip::udp::v4());

        boost::array<unsigned char, 48> sendBuf = {{10,0,0,0,0,0,0,0,0}};

        socket.send_to(boost::asio::buffer(sendBuf), receiver_endpoint);

        boost::array<unsigned long, 1024> recvBuf;
        ip::udp::endpoint sender_endpoint;

        try
        {

            fd_set fileDescriptorSet;
            struct timeval timeStruct;

            // set the timeout to 5 seconds
            timeStruct.tv_sec = GetArg("-ntptimeout", 5);
            timeStruct.tv_usec = 0;
            FD_ZERO(&fileDescriptorSet);

            int nativeSocket = socket.native();

            FD_SET(nativeSocket,&fileDescriptorSet);

            select(nativeSocket+1,&fileDescriptorSet,NULL,NULL,&timeStruct);

            if(!FD_ISSET(nativeSocket,&fileDescriptorSet))
            {

                LogPrint("ntp", "CNtpClient: Could not read socket from NTP server %s (Read timeout)\n", sHostName);

            }
            else
            {

                socket.receive_from(boost::asio::buffer(recvBuf), sender_endpoint);
                timeRecv = ntohl((time_t)recvBuf[4]);
                timeRecv-= 2208988800U;  // Substract 01/01/1970 == 2208988800U

                LogPrint("ntp", "CNtpClient: Received timestamp: %ll", (long)timeRecv);

            }

        }
        catch (std::exception& e)
        {

             LogPrintf("CNtpClient: Could not read clock from NTP server %s (%s)\n", sHostName, e.what());

        }

    }
    catch (std::exception& e)
    {

        LogPrintf("CNtpClient: Could not open socket to NTP server %s (%s)\n", sHostName, e.what());

    }



    return (int64_t)timeRecv;
  }
