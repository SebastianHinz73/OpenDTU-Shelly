// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "RamDataType.h"
// #include "WebsocketServer.h"

#include <iostream>
#include <set>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <thread>

#define TESTSERVER_PORT 80

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

class TestServer {
public:
    TestServer();
    ~TestServer();
    void Start();
    void Stop();
    void WaitStarted();
    void Update(RamDataType_t type, float value);

private:
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl, server::message_ptr msg);
    void run(uint16_t port);
    void stop();

private:
    typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;
    server m_server;
    con_list m_connections;
    con_list _connPro;
    con_list _connPlugS;
    std::mutex _connMutex;

private:
    std::thread* _runThread;
};
