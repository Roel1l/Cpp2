//=============================================================================
// socket.cpp
// (c) 8-12-2010, Frans Spijkerman, Avans Hogeschool
// enhanced for cross-platform use by Bob Polis (c) 2013, 2014
//
// Implementation of classes Socket, ServerSocket and CientSocket
//=============================================================================


#include "Socket.h"
#include "throw.h"
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <exception>
#include <memory>
#include <sstream>
#include <system_error>

#if HAVE_POSIX

    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <net/route.h>
    #include <net/if.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/types.h>
    #include <netdb.h>
    #include <ifaddrs.h>
    #include <unistd.h>
    #include <fcntl.h>

#else // Windows

    #include <ws2tcpip.h>

    // Tell linker to use these libraries
    #pragma comment (lib, "Ws2_32.lib")
    #pragma comment (lib, "Mswsock.lib")
    #pragma comment (lib, "AdvApi32.lib")

    //=============================================================================
    class WSA
    //=============================================================================
    // ONLY NEEDED IN MS-WINDOWS
    //
    // An instance of this class is created globally.
    // So constructor is automatically called before main()
    // and destructor is automatically called after main().
    // Result: Windows Socket API is active when needed.
    //=============================================================================
    {
    private:
        WSADATA data;
    public:
        WSA()
        {
            int iResult = WSAStartup(MAKEWORD(2, 2), &data);
            if (iResult != 0) {
                std::cerr << "WSAStartup failed with error: " << iResult << '\n';
            }
        }
        ~WSA()
        {
            WSACleanup();
        }
    } wsa; // instance

#endif // Windows

#pragma mark Socket


void throw_unless_would_block() {
#if HAVE_POSIX
    if (errno != EWOULDBLOCK && errno != EAGAIN)  {
        throw std::system_error { errno, std::system_category() };
    }
#else
    int err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK) {
        throw std::system_error { err, std::system_category() };
    }
#endif
    
}

Socket::Socket(SOCKET sock, const struct sockaddr& address) : sock {sock}
{
    switch (address.sa_family) {
        case AF_INET: {
            struct sockaddr_in* in_addr = reinterpret_cast<struct sockaddr_in*>(&addr);
            const struct sockaddr_in* ipv4 = reinterpret_cast<const struct sockaddr_in*>(&address);
            *in_addr = *ipv4;
            break;
        }
        case AF_INET6: {
            struct sockaddr_in6* in6_addr = reinterpret_cast<struct sockaddr_in6*>(&addr);
            const struct sockaddr_in6* ipv6 = reinterpret_cast<const struct sockaddr_in6*>(&address);
            *in6_addr = *ipv6;
            break;
        }
        default: {
            throw std::runtime_error("invalid type of socket address");
        }
    }
}

Socket::Socket(Socket&& other)
{
    sock = other.sock;
    addr = other.addr;
    other.sock = 0;
}

Socket& Socket::operator=(Socket&& other)
{
    if (this != &other) {
        if (sock > 0) close();
        sock = other.sock;
        addr = other.addr;
        other.sock = 0;
    }
    return *this;
}

ssize_t Socket::read(char *buf, size_t maxlen) const
{
    ssize_t len = 0;
    // might come in parts
    while (ssize_t n = ::recv(sock, buf + len, int(maxlen - len), 0)) {
        if (n < 0) {
            throw_unless_would_block();
            break;
        }
        len += n;
        if (len >= maxlen) break;
    }
    return len;
}

char Socket::read() const
{
    char c;
    read(&c, 1);
    return c;
}


void Socket::write(const std::string& msg) const
{
    write(msg.c_str(), msg.length());
}

void Socket::write(const char *buf, size_t len) const
{
    throw_if_min1((int)::send(sock, buf, (int)len, 0));
}

void Socket::write(char c) const
{
    write(&c, 1);
}

const Socket& Socket::operator<<(const char c) const
{
    write(c);
    return *this;
}

const Socket& Socket::operator<<(const char* message) const
{
    write(std::string {message});
    return *this;
}

const Socket& Socket::operator<<(const std::string& message) const
{
    write(message);
    return *this;
}

Socket::~Socket()
{
    if (sock > 0) close();
}

void Socket::close()
{
    std::cerr << "will close socket " << sock << '\n';
#if defined(__APPLE__) || defined(__linux__)
    throw_if_min1(::close(sock));
#else
    ::closesocket(sock);
#endif
    sock = 0;
}

std::string Socket::get_dotted_ip() const
{
    const char* result {nullptr};
    char textbuf[INET6_ADDRSTRLEN]; // large enough for both IPv4 and IPv6 addresses
    const struct sockaddr* addr_p = reinterpret_cast<const struct sockaddr*>(&addr);
    switch (addr_p->sa_family) {
        case AF_INET: {
            const struct sockaddr_in* in_p = reinterpret_cast<const struct sockaddr_in*>(addr_p);
            throw_if_null(result = ::inet_ntop(AF_INET, (void*)&in_p->sin_addr, textbuf, INET6_ADDRSTRLEN));
            break;
        }
        case AF_INET6: {
            const struct sockaddr_in6* in6_p = reinterpret_cast<const struct sockaddr_in6*>(addr_p);
            throw_if_null(result = ::inet_ntop(AF_INET6, (void*)&in6_p->sin6_addr, textbuf, INET6_ADDRSTRLEN));
            break;
        }
        default:
            return "n/a";
    }
    return result;
}

#pragma mark ServerSocket

ServerSocket::ServerSocket(int port)
{

    throw_if_min1(sock = ::socket(AF_INET, SOCK_STREAM, 0));
#if HAVE_POSIX
	int flags = fcntl(sock, F_GETFL, NULL);
    errno = 0;
    if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		throw std::system_error { errno, std::system_category() };
    }
#else
    u_long blocking = 1;
    int result = ioctlsocket(sock, FIONBIO, &blocking);
    if (result != NO_ERROR) {
        throw std::system_error { WSAGetLastError(), std::system_category() };
    }
#endif

    struct sockaddr_in saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = INADDR_ANY;
    saServer.sin_port = htons(port);

    throw_if_min1(::bind(sock, (struct sockaddr*)&saServer, sizeof(struct sockaddr)));
    throw_if_min1(::listen(sock, 100));  // the number of clients that can be queued
}

bool ServerSocket::accept_nonblock(SOCKET &sock_out, sockaddr &client_addr) {
    client_addr.sa_family = AF_INET;
    socklen_t len = sizeof(client_addr);
    sock_out = ::accept(sock, &client_addr, &len);
#if HAVE_POSIX
    if (sock_out < 0) {
#else
    if (sock_out == INVALID_SOCKET) {
#endif
        throw_unless_would_block();
        return false;
    }
   return true;
}

#pragma mark ClientSocket

ClientSocket::ClientSocket(const std::string& host, int port)
{
    // construct network address for server
    struct addrinfo hint;
    std::memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    struct addrinfo* infolist {nullptr};
    int gai_error = ::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hint, &infolist);
    if (gai_error) {
        std::ostringstream oss;
        oss << "getaddrinfo error " << gai_error << ": " << gai_strerror(gai_error) << " (" << __FILE__ << ":" << __LINE__ << ")";
        throw std::runtime_error(oss.str());
    }
    // wrap our list pointer inside unique_ptr for auto cleanup
#if defined(__APPLE__) || defined(__linux__)
    using cleanup_func = void(*)(struct addrinfo*);
    std::unique_ptr<struct addrinfo, cleanup_func> list {infolist, ::freeaddrinfo};
#else // Windows
    using cleanup_func = void(__stdcall*)(PADDRINFOA);
    std::unique_ptr<struct addrinfo, cleanup_func> list(infolist, ::freeaddrinfo);
#endif
    // create socket
    throw_if_min1(sock = ::socket(list->ai_family, list->ai_socktype, list->ai_protocol));

    // connect to server
    throw_if_min1(::connect(sock, (struct sockaddr*)list->ai_addr, list->ai_addrlen));
}
