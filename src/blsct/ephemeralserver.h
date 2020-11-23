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

class connection_manager;

class tcp_connection
{
public:
    tcp_connection(boost::asio::io_context& io_context, cb_t data_cb_in, connection_manager& manager)
        : socket_(io_context), data_cb(data_cb_in), connection_manager_(manager)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start();

    void stop();

    void handle_read(const boost::system::error_code& error,
                     size_t bytes_transferred);

private:
    connection_manager& connection_manager_;
    boost::asio::streambuf b;
    tcp::socket socket_;
    cb_t data_cb;
};

class connection_manager
        : public boost::enable_shared_from_this<connection_manager>
{
public:
    /// Add the specified connection to the manager and start it.
    void start(tcp_connection* c);

    /// Stop the specified connection.
    void stop(tcp_connection* c);

    /// Stop all connections.
    void stop_all();

private:
    /// The managed connections.
    std::set<tcp_connection*> connections_;
};


class EphemeralSession
{
public:
    EphemeralSession(boost::asio::io_context& io_context, cb_t data_cb_in)
        : io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), 0)), data_cb(data_cb_in), connection_manager_()
    {
        port = acceptor_.local_endpoint().port();
        StartAccept();
    }

    void Stop();

    unsigned short port;

private:
    void StartAccept();

    void HandleAccept(tcp_connection* new_connection,
                      const boost::system::error_code& error);

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    connection_manager connection_manager_;
    cb_t data_cb;
};

class EphemeralServer
{
public:
    EphemeralServer(hs_cb_t hs_cb_in, cb_t data_cb_in, int timeout = 120);

    void Start();
    void Stop();
    bool IsRunning() const;

private:
    int live_until;
    bool fState;

    boost::thread ephemeralServerThread;
    boost::promise<std::string> *p;
    cb_t data_cb;
    hs_cb_t hs_cb;

    TorControlThread torController;

    void SetHiddenService(std::string s);
};

#endif // EPHEMERALSERVER_H
