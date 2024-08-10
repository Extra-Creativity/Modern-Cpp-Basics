#include "TCPStream.h"
#include <iostream>
#include <print>
#include <string_view>

int main()
{
    Network::Startup();
    Network::Socket connectSocket{ "127.0.0.1", 34567,
                                   Network::Socket::Tag::Connect };
    if (!connectSocket)
    {
        std::println("Connect socket error: {}", Network::GetErrorCode());
        return 1;
    }

    Network::OTCPStream stream;
    stream.open(std::move(connectSocket), 4);
    std::string str;
    for (int i = 0; i < 10; i++)
    {
        std::cin >> str;
        // 这里改成endl是为了让接收方输入字串在空白处停止，否则会一直尝试读入。
        stream << str << std::endl;
    }

    return 0;
}