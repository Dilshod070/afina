#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <iostream>

#include <sys/epoll.h>

#include <afina/execute/Command.h>
#include <afina/Storage.h>
#include "protocol/Parser.h"

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s) : _socket(s), _alive(false)
    {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    inline bool isAlive() const { return _alive; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    // std::shared_ptr<spdlog::logger> _logger;
    // std::shared_ptr<Afina::Storage> pStorage;

    bool _alive;
    int _socket;
    struct epoll_event _event;

    Protocol::Parser parser;
    std::size_t arg_remains;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    
    char client_buffer[256];
    std::size_t _bytes_left;

    std::vector<std::string> _answers;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
