// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ephemeralserver.h"


EphemeralServer::EphemeralServer(hs_cb_t hs_cb_in, cb_t data_cb_in, int timeout) :
    live_until(timeout), hs_cb(hs_cb_in), data_cb(data_cb_in), fState(0)
{
    ephemeralServerThread = boost::thread(boost::bind(&EphemeralServer::Start, this));
}

bool EphemeralServer::IsRunning() const
{
    return fState;
}

void EphemeralServer::Stop()
{
    fState = 0;
}

void EphemeralServer::Start()
{
    fState = 1;

    boost::asio::io_service io_service;
    EphemeralSession s(io_service, data_cb);

    LogPrint("ephemeralserver", "EphemeralServer::%s: Listening on %d\n", __func__, s.port);

    try
    {
        torController = TorControlThread(s.port, [=](std::string s) {
            SetHiddenService(s);
        });
        torController.Start();

        boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

        live_until += GetTime();

        while (fState == 1 && GetTime() < live_until)
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }

        fState = 0;

        s.Stop();
        torController.Stop();
        t.join();
    }
    catch(...)
    {
        fState = 0;

        s.Stop();
        torController.Stop();
    }

    LogPrint("ephemeralserver", "EphemeralServer::%s: Closed ephemeral server at port %d\n", __func__, s.port);
}

void EphemeralServer::SetHiddenService(std::string s)
{
    hs_cb(s);
}

void tcp_connection::start()
{
    boost::asio::async_read(socket_, b, boost::asio::transfer_all(),
                                  boost::bind(&tcp_connection::handle_read, shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
}

void tcp_connection::stop()
{
    socket_.close();
}

void tcp_connection::handle_read(const boost::system::error_code& error,
                                 size_t bytes_transferred)
{
    if (bytes_transferred > 0)
    {
        std::vector<unsigned char> result(
                    buffers_begin(b.data()),
                    buffers_begin(b.data()) + bytes_transferred
                    - 1);

        b.consume(bytes_transferred);
        data_cb(result);
    }
    else if (error != boost::asio::error::operation_aborted)
    {
        connection_manager_.stop(shared_from_this());
    }
}

void EphemeralSession::Stop()
{
    acceptor_.close();
    connection_manager_.stop_all();
}


void EphemeralSession::HandleAccept(connection_ptr new_connection,
                  const boost::system::error_code& error)
{
    if (!acceptor_.is_open())
    {
        return;
    }

    if (!error)
    {
        connection_manager_.start(new_connection);
    }

    StartAccept();
}

void EphemeralSession::StartAccept()
{
    new_connection_.reset(new tcp_connection(io_context_, data_cb, connection_manager_));

    acceptor_.async_accept(new_connection_->socket(),
                           boost::bind(&EphemeralSession::HandleAccept, this, new_connection_,
                                       boost::asio::placeholders::error));
}

void connection_manager::start(connection_ptr c)
{
    connections_.insert(c);
    c->start();
}

void connection_manager::stop(connection_ptr c)
{
    connections_.erase(c);
    c->stop();
}

void connection_manager::stop_all()
{
    std::for_each(connections_.begin(), connections_.end(),
                  boost::bind(&tcp_connection::stop, _1));
    connections_.clear();
}
