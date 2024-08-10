#include "TCPStream.h"
#include <iostream>
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

    Network::ITCPStream stream;
    stream.open(std::move(acceptSock), 4);
    std::string str;
    while (stream >> str)
    {
        std::cout << str << std::endl;
    }
    return 0;
}