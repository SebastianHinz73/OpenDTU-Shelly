#include "TestServer.h"
// https://github.com/zaphoyd/websocketpp/issues/478
// from https://github.com/adamrehn/websocket-server-demo/tree/master
// https://github.com/zaphoyd/websocketpp/issues/403

TestServer::TestServer()
    : _senderThread(nullptr)
    , _runThread(nullptr)
{
    m_server.init_asio();

    m_server.set_open_handler(bind(&TestServer::on_open, this, ::_1));
    m_server.set_close_handler(bind(&TestServer::on_close, this, ::_1));
    m_server.set_message_handler(bind(&TestServer::on_message, this, ::_1, ::_2));

    // m_server.clear_access_channels(websocketpp::log::alevel::all);
    m_server.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);
}

TestServer::~TestServer()
{
    Stop();
}

void TestServer::on_open(connection_hdl hdl)
{
    m_connections.insert(hdl);
}

void TestServer::on_close(connection_hdl hdl)
{
    m_connections.erase(hdl);
}

void TestServer::on_message(connection_hdl hdl, server::message_ptr msg)
{
    auto payload = msg->get_payload();
    auto id = payload.substr(6, 1);

    if (std::stoi(id) == 1) {
        _connPro.insert(hdl);
    } else {
        _connPlugS.insert(hdl);
    }
}

void TestServer::run(uint16_t port)
{
    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
}

void TestServer::sendTest()
{
    int cnt = 20;
    while (cnt-- > 0) {
        for (auto it : m_connections) {
            m_server.send(it, "TEEEEEST", websocketpp::frame::opcode::text);
        }
        Sleep(300);
    }
}

void TestServer::stop()
{
    m_server.stop_listening();

    for (auto it : m_connections) {
        m_server.pause_reading(it);
        m_server.close(it, 0, "");
    }
}

void TestServer::Start()
{
    //_senderThread = new std::thread([this]() {
    //    sendTest();
    //});

    _runThread = new std::thread([this]() {
        run(80);
        printf("Servers stopped\r\n");
        fflush(stdout);
    });
}

void TestServer::Stop()
{
    if (_senderThread != nullptr) {
        _senderThread->join();
        delete _senderThread;
        _senderThread = nullptr;
    }

    if (_runThread != nullptr) {
        printf("server.stop 1\r\n");
        fflush(stdout);
        stop();

        _runThread->join();
        delete _runThread;
        _runThread = nullptr;
    }
}

void TestServer::Update(RamDataType_t type, float value)
{
    static char b[64];

    switch (type) {
    case RamDataType_t::Pro3EM:
        sprintf(b, "\"total_act_power\":%.3f,", value);

        if (_connPro.size() > 0) {
            m_server.send(*_connPro.begin(), b, websocketpp::frame::opcode::text);
        }
        break;
    }
}
