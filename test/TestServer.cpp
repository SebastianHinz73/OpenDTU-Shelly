#include "TestServer.h"
// https://github.com/zaphoyd/websocketpp/issues/478
// from https://github.com/adamrehn/websocket-server-demo/tree/master
// https://github.com/zaphoyd/websocketpp/issues/403

TestServer::TestServer()
    : _runThread(nullptr)
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

    {
        std::lock_guard<std::mutex> lock(_connMutex);

        if (std::stoi(id) == 1) {
            _connPro.insert(hdl);
        } else {
            _connPlugS.insert(hdl);
        }
    }
    m_server.send(hdl, "opendtu_shelly_debug", websocketpp::frame::opcode::text);
}

void TestServer::run(uint16_t port)
{
    m_server.listen(port);
    m_server.start_accept();
    m_server.run();
}

void TestServer::stop()
{
    m_server.stop_listening();

    {
        std::lock_guard<std::mutex> lock(_connMutex);
        for (auto it : m_connections) {
            m_server.pause_reading(it);
            m_server.close(it, 0, "");
        }
    }
}

void TestServer::WaitStarted()
{
    while (1) {
        bool bExit = false;
        {
            std::lock_guard<std::mutex> lock(_connMutex);
            bExit = _connPro.size() > 0 && _connPlugS.size() > 0;
        }
        if (bExit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void TestServer::Start()
{
    _runThread = new std::thread([this]() {
        run(TESTSERVER_PORT);
    });
}

void TestServer::Stop()
{
    if (_runThread != nullptr) {
        stop();
        std::this_thread::sleep_for(std::chrono::microseconds(1000));

        _runThread->join();
        delete _runThread;
        _runThread = nullptr;
    }
}

void TestServer::Update(RamDataType_t type, float value)
{
    std::lock_guard<std::mutex> lock(_connMutex);

    static char b[64];

    switch (type) {
    case RamDataType_t::Pro3EM:
        if (_connPro.size() > 0) {
            sprintf(b, "::Pro3EM:%.3f,", value);
            m_server.send(*_connPro.begin(), b, websocketpp::frame::opcode::text);
        }
        break;
    case RamDataType_t::Pro3EM_Min:
        if (_connPro.size() > 0) {
            sprintf(b, "::Pro3EM_Min:%.3f,", value);
            m_server.send(*_connPro.begin(), b, websocketpp::frame::opcode::text);
        }
        break;
    case RamDataType_t::Pro3EM_Max:
        if (_connPro.size() > 0) {
            sprintf(b, "::Pro3EM_Max:%.3f,", value);
            m_server.send(*_connPro.begin(), b, websocketpp::frame::opcode::text);
        }
        break;
    case RamDataType_t::PlugS:
        if (_connPlugS.size() > 0) {
            sprintf(b, "::PlugS:%.3f,", value);
            m_server.send(*_connPlugS.begin(), b, websocketpp::frame::opcode::text);
        }
        break;
    case RamDataType_t::CalulatedLimit:
        if (_connPlugS.size() > 0) {
            sprintf(b, "::CalulatedLimit:%.3f,", value);
            m_server.send(*_connPlugS.begin(), b, websocketpp::frame::opcode::text);
        }
        break;
    case RamDataType_t::Limit:
        if (_connPlugS.size() > 0) {
            sprintf(b, "::Limit:%.3f,", value);
            m_server.send(*_connPlugS.begin(), b, websocketpp::frame::opcode::text);
        }
        break;
    };
}
