#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>

#include <sys/epoll.h>

#include "protocol/Parser.h"
#include <afina/Storage.h>
#include <afina/execute/Command.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> store) : _socket(s), _alive(false) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        pStorage = store;
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
    std::shared_ptr<Afina::Storage> pStorage;

    bool _alive;
    int _socket;
    struct epoll_event _event;

    Protocol::Parser parser;
    std::size_t arg_remains;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;

    std::size_t _read_queue_size;
    char _read_buffer[256];
    std::size_t _sent_last;

    std::vector<std::string> _answers;
    std::mutex _answ_mutex;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
