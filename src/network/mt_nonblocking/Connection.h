#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

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
namespace MTnonblock {

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
    friend class Worker;
    friend class ServerImpl;

    std::shared_ptr<Afina::Storage> pStorage;

    Protocol::Parser parser;
    std::size_t arg_remains;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;

    bool _alive;
    int _socket;
    struct epoll_event _event;

    std::size_t _read_queue_size;
    char _read_buffer[256];
    std::size_t _write_queue_size;
    std::size_t _sent_last;
    char _write_buffer[256];

    // std::vector<std::string> _answers;
    std::queue<std::string> _answers;
    std::mutex _answ_mutex;
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
