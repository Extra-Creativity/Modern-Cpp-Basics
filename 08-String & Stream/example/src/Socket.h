#pragma once

#ifdef _WIN32

#define NOMINMAX
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

#else

#include <netinet/in.h>
#include <sys/socket.h>

#endif

#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace Network
{

class Socket
{
#ifdef _WIN32
    inline static constexpr SOCKET s_invalidSocket_ = INVALID_SOCKET;
    SOCKET socket_{ s_invalidSocket_ };
#else
    inline static constexpr int s_invalidSocket_ = -1;
    int socket_{ s_invalidSocket_ };
#endif

public:
    enum class Tag
    {
        Listen,
        Accept,
        Connect
    };

    Socket() = default;

    // Listen socket or connect socket.
    Socket(const char *ip, std::uint16_t port, Tag tag);

    // Accept socket; To distinguish it from copy ctor (in fact socket doesn't
    // have it), add a tag param.
    Socket(const Socket &listenSock, Tag tag);

    Socket(Socket &&another) noexcept
        : socket_{ std::exchange(another.socket_, s_invalidSocket_) }
    {
    }
    Socket &operator=(Socket &&another) noexcept
    {
        if (*this)
            Clean_();
        socket_ = std::exchange(another.socket_, s_invalidSocket_);
        return *this;
    }

    ~Socket()
    {
        if (*this)
            Clean_();
    }

    // Clean and reset.
    void Close()
    {
        if (*this)
        {
            Clean_();
            socket_ = s_invalidSocket_;
        }
    }
    explicit operator bool() const noexcept
    {
        return socket_ != s_invalidSocket_;
    }
    auto GetHandle() const noexcept { return socket_; }

private:
    bool CreateSocketCommon_(const char *ip, std::uint16_t port,
                             sockaddr_in &addr);
    void CreateListenSocket_(const char *ip, std::uint16_t port);
    void CreateConnectSocket_(const char *ip, std::uint16_t port);
    void Clean_();
};

inline auto GetErrorCode()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline bool Startup()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
    {
        return false;
    }
#endif
    return true;
}

} // namespace Network
