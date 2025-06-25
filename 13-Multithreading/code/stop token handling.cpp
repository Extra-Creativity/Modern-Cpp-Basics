#include <thread>
#include <iostream>

int main()
{
    using namespace std::literals;
    std::jthread t{ [](std::stop_token token) {
        while (!token.stop_requested())
        {
            std::this_thread::sleep_for(100ms);
            std::cout << "PKU No.1\n";
        }
    } };
    
    std::stop_callback callback{ t.get_stop_token(),
        []() { std::cout << "THU No.2\n"; }
    };

    std::this_thread::sleep_for(500ms);

    return 0;
}