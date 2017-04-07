#pragma once
//=============================================================================
// socket.h
// (c) 8-12-2010, Frans Spijkerman, Avans Hogeschool
// enhanced for cross-platform use by Bob Polis (c) 2013, 2014
// modified accept to return object by Jeroen de Haas (c) 2016
//
// Definition of classes Socket, ServerSocket and CientSocket
//
// Socket:
//  Abstraction of tcp-socket using Windows Sockets API (WSA)
//
// ServerSocket:
//  Socket to listen to clients on a tcp-port
//
// ClientSocket:
//  Socket to connect to a server on a tcp-port
//=============================================================================
//

#define HAVE_POSIX defined(__APPLE__) || defined(__linux__)

#if HAVE_POSIX
    #include <sys/socket.h>
    typedef int SOCKET;
#else // Windows
    #include <winsock2.h>
    typedef long ssize_t;
#endif

#include <iostream>
#include <sstream>
#include <string>
#include "CircularBuffer.h"

void throw_unless_would_block();

//=============================================================================
class Socket
//=============================================================================
{
protected:
    SOCKET sock;
    struct sockaddr_storage addr;
    mutable CircularBuffer<char> buffer; 
public:
    Socket() : sock(0), buffer {512} {}
    Socket(SOCKET sock, const struct sockaddr& address);
    Socket(const Socket& other) = delete;
    Socket(Socket&& other);
    virtual ~Socket();

    Socket& operator=(const Socket& other) = delete;
    Socket& operator=(Socket&& other);

    SOCKET get_socket() const { return sock; }
    void set_socket(SOCKET sock) { this->sock = sock; }
    void close();
    bool is_open() const { return sock > 0; }
    std::string get_dotted_ip() const;

    ssize_t read(char *buf, size_t maxlen) const;
    char read() const;
    std::string readline() const;

    void write(const std::string& msg) const;
    void write(const char *buf, size_t len) const;
    void write(char c) const;

    const Socket& operator<<(const char c) const;
    const Socket& operator<<(const char* message) const;
    const Socket& operator<<(const std::string& message) const;

    template <typename T> const Socket& operator<<(T value) const {
        write(std::to_string(value));
        return *this;
    }
    template<typename Callback>
    bool readline(Callback callback) const {
        char c;
        while (ssize_t n = ::recv(sock, &c, 1, 0)) {
            if (n == 0) {
                return false;
            }
            else if (n < 0) {
                throw_unless_would_block();
                break;
            }

            if (c == '\n') {
                std::stringstream s { "" };
                for(int i { 0 }; i < buffer.size(); ++i)
                    s << buffer[i];
                buffer.clear();
                callback(s.str());
                return true;
            }
            if (c != '\r') {
                buffer.push_back(c);
            }
        }
        return false;
    }
};

//=============================================================================
class ServerSocket : public Socket
//=============================================================================
{
private:
    bool accept_nonblock(SOCKET &sock, sockaddr &client_addr);
public:
    ServerSocket(int port);
    template<typename Callback>
    void accept(Callback callback) {
        SOCKET fd;
        sockaddr client_addr;
        if (accept_nonblock(fd, client_addr)) {
            callback(Socket { fd, client_addr });
        }
    };
};

//=============================================================================
class ClientSocket : public Socket
//=============================================================================
{
public:
    ClientSocket(const std::string& host, int port);
};

