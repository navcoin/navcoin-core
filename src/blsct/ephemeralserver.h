// Copyright (c) 2020 The NavCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EPHEMERALSERVER_H
#define EPHEMERALSERVER_H

#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio.hpp>

#include <torcontrol.h>
#include <util.h>

using boost::asio::ip::tcp;

typedef std::function<void(std::vector<unsigned char>&)> cb_t;
typedef std::function<void(std::string&)> hs_cb_t;

class EphemeralServer
{
public:
    EphemeralServer(hs_cb_t hs_cb_in, cb_t data_cb_in, int timeout = 120);

    void Start();
    void Stop();
    bool IsRunning() const;

private:
    void Accept();

    int live_until;
    bool fState;

    boost::thread ephemeralServerThread;
    boost::promise<std::string> *p;
    cb_t data_cb;
    hs_cb_t hs_cb;

    TorControlThread torController;

    void SetHiddenService(std::string s);
};

class tcp_connection
{
public:
    tcp_connection(boost::asio::io_context& io_context, cb_t data_cb_in)
        : socket_(io_context), data_cb(data_cb_in)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start();

    void handle_read(const boost::system::error_code& error,
                     size_t bytes_transferred);

private:
    boost::asio::streambuf b;
    tcp::socket socket_;
    cb_t data_cb;
};

class EphemeralSession
{
public:
    EphemeralSession(boost::asio::io_context& io_context, cb_t data_cb_in)
        : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), 0)), data_cb(data_cb_in)
    {
        port = acceptor_.local_endpoint().port();
        Accept();
    }

    unsigned short port;

private:
    void Accept()
    {
        tcp_connection* new_connection = new
                tcp_connection(io_context_, data_cb);

        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&EphemeralSession::HandleAccept, this, new_connection,
                                           boost::asio::placeholders::error));
    }

    void HandleAccept(tcp_connection* new_connection,
                      const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }
        else
        {
            delete new_connection;
        }

        Accept();
    }

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    cb_t data_cb;
};

#endif // EPHEMERALSERVER_H
