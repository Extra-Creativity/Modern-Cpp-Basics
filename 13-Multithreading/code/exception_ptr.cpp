#include <thread>
#include <iostream>
#include <exception>

void THUStudent() { 
    throw std::runtime_error("THU is not the best!");
    std::cout << "THU is the best.\n";
}

void PKUStudent() {
    std::cout << "PKU is the best.\n";
}

void Work(std::exception_ptr& ptr)
{
    try {
        PKUStudent();
        THUStudent();
    }
    catch (const std::runtime_error&) {
        ptr = std::current_exception();
    }
    return;
}

void Watch()
{
    std::exception_ptr ptr;
    // join immediately.
    {std::jthread _{ Work, std::ref(ptr) }; }
    if (ptr)
        std::rethrow_exception(ptr);
    std::cout << "All students over.\n";
}

int main()
{
    try {
        Watch();
    }
    catch (const std::runtime_error& error) {
        std::cout << error.what();
    }

    return 0;
}