#include "Socket.h"
#include <stdexcept>

#ifdef _WIN32

#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#else

#include <arpa/inet.h>
#include <unistd.h>

#endif

namespace Network
{

Socket::Socket(const char *ip, std::uint16_t port, Tag tag)
{
    if (tag == Tag::Listen)
    {
        CreateListenSocket_(ip, port);
    }
    else if (tag == Tag::Connect)
    {
        CreateConnectSocket_(ip, port);
    }
    else [[unlikely]]
    {
        throw std::runtime_error{ "Unknown tag.\n" };
    }
}

Socket::Socket(const Socket &listenSock, Tag tag)
{
    if (tag != Tag::Accept) [[unlikely]]
    {
        throw std::runtime_error{ "Unknown tag.\n" };
    }

    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    socket_ = ::accept(listenSock.socket_, reinterpret_cast<sockaddr *>(&addr),
                       &addrLen);
}

bool Socket::CreateSocketCommon_(const char *ip, std::uint16_t port,
                                 sockaddr_in &addr)
{
    addr = {};
    addr.sin_family = AF_INET;
    if (::inet_pton(AF_INET, ip, &addr.sin_addr) == 0) // Wrong IP format.
    {
        return false;
    }
    addr.sin_port = ::htons(port);
    socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return socket_ != s_invalidSocket_;
}

void Socket::CreateListenSocket_(const char *ip, std::uint16_t port)
{
    sockaddr_in addr;
    if (!CreateSocketCommon_(ip, port, addr))
    {
        return;
    }

    if (::bind(socket_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
        -1)
    {
        Close();
    }

    if (::listen(socket_, -1) == -1)
    {
        Close();
    }
}

void Socket::CreateConnectSocket_(const char *ip, std::uint16_t port)
{
    sockaddr_in addr;
    if (!CreateSocketCommon_(ip, port, addr))
    {
        return;
    }

    if (::connect(socket_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
        -1)
    {
        Close();
    }
}

void Socket::Clean_()
{
#ifdef _WIN32
    ::closesocket(socket_);
#else
    ::close(socket_);
#endif
}

} // namespace Network