#include "Socket.h"
#include <print>
#include <string_view>

int main()
{
    Network::Startup();
    Network::Socket socket{ "127.0.0.1", 34567, Network::Socket::Tag::Listen };
    if (!socket)
    {
        std::println("Listen socket error: {}", Network::GetErrorCode());
        return 1;
    }
    Network::Socket acceptSock{ socket, Network::Socket::Tag::Accept };
    if (!acceptSock)
    {
        std::println("Accept socket error: {}", Network::GetErrorCode());
        return 1;
    }

    char str[1024];
    while (true)
    {
        int len = ::recv(acceptSock.GetHandle(), str, 1024, 0);
        if (len <= 0)
            break;
        std::string_view content{ str, str + len };
        std::println("Recv from server: {}", content);
    }
    return 0;
}