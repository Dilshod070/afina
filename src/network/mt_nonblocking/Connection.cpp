#include "Connection.h"

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace Afina {
namespace Network {
namespace MTnonblock {

// See Connection.h
void Connection::Start()
{
	std::cout << "Start" << std::endl;

	_read_queue_size = 0;
	_write_queue_size = 0;
	_sent_last = 0;

	_event.data.fd = _socket;
	_event.data.ptr = this;
	_event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLONESHOT;
}

// See Connection.h
void Connection::OnError()
{ 
	std::cout << "OnError" << std::endl;
	_alive = false;
}

// See Connection.h
void Connection::OnClose()
{
	std::cout << "OnClose" << std::endl;
	_alive = false;
}

// See Connection.h
void Connection::DoRead()
{
	std::cout << "DoRead" << std::endl;
	try {
        int readed_bytes = -1;
        if ((readed_bytes = read(_socket, _read_buffer + _read_queue_size, sizeof(_read_buffer) - _read_queue_size)) > 0) {
        	_read_queue_size += readed_bytes;
            // _logger->debug("Got {} bytes from socket", _read_queue_size);

            // Single block of data readed from the socket could trigger inside actions a multiple times,
            // for example:
            // - read#0: [<command1 start>]
            // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
            while (_read_queue_size > 0) {
                // _logger->debug("Process {} bytes", _read_queue_size);
                // There is no command yet
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    if (parser.Parse(_read_buffer, _read_queue_size, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
                        // _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                        command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(_read_buffer, _read_buffer + parsed, _read_queue_size - parsed);
                        _read_queue_size -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (command_to_execute && arg_remains > 0) {
                    // _logger->debug("Fill argument: {} bytes of {}", _read_queue_size, arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(arg_remains, std::size_t(_read_queue_size));
                    argument_for_command.append(_read_buffer, to_read);

                    std::memmove(_read_buffer, _read_buffer + to_read, _read_queue_size - to_read);
                    arg_remains -= to_read;
                    _read_queue_size -= to_read;
                }

                // Thre is command & argument - RUN!
                if (command_to_execute && arg_remains == 0) {
                    // _logger->debug("Start command execution");

                    std::string result;
                    command_to_execute->Execute(*pStorage, argument_for_command, result);

                    // Send response
                    result += "\r\n";
                    {
                    	std::lock_guard<std::mutex> guard(_answ_mutex);
                    	_answers.push(result);
                    }
                    _write_queue_size += result.size();
                    _event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLONESHOT;

                    // Prepare for the next command
                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();
                }
            } // while (_read_queue_size > 0)
        }

        if (readed_bytes == 0) {
            // _logger->debug("Connection closed");
        } else {
            throw std::runtime_error(std::string(strerror(errno)));
        }
    } catch (std::runtime_error &ex) {
        // _logger->error("Failed to process connection on descriptor {}: {}", client_socket, ex.what());
    }
}

// See Connection.h
void Connection::DoWrite()
{
	std::cout << "DoWrite" << std::endl;
	auto result = _answers.front();
	std::memcpy(_write_buffer, result.c_str(), result.size());
	int sent = send(_socket, _write_buffer + _sent_last, result.size() - _sent_last, 0);
	_write_queue_size -= sent;
	_sent_last += sent;
	if (_sent_last == result.size()) {
		_sent_last = 0;
		std::lock_guard<std::mutex> guard(_answ_mutex);
		_answers.pop();
		if (_answers.empty()) {
			_event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLONESHOT;
		}
	}
}

} // namespace MTnonblock
} // namespace Network
} // namespace Afina
