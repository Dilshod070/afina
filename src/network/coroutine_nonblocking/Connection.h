#ifndef AFINA_NETWORK_COROUTINE_CONNECTION_H
#define AFINA_NETWORK_COROUTINE_CONNECTION_H

#include <afina/coroutine/Engine.h>
#include <atomic>

namespace Afina {
namespace Network {
namespace Coroutine {

struct Connection;

struct Connection {
    Connection() : prev(nullptr), next(nullptr), ctx(nullptr), events(0), running(false){};
    Connection *prev;
    Connection *next;
    Afina::Coroutine::Engine::context *ctx;
    uint32_t events;
    std::atomic_bool running;
};
} // namespace Coroutine
} // namespace Network
} // namespace Afina

#endif
