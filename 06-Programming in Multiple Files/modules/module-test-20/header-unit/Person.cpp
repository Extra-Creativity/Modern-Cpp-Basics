module;
#include <iostream>
#include <format>
#include <string>

module Person; // denote that it's a module implementation
import "Old.h";

Person::Person(const std::string& init_name, std::uint64_t init_id):
    name{init_name}, id{init_id} {}

void Person::Print() const
{
    std::cout << std::format("Person #{}: {}\n", id, name);
}

void InnerMethod();

// No need to specify any export; it's the duty of module interface!
void HelloWorld()
{
    SomeOldLibFunc(1);
    std::cout << "Hello, world!\n";
    std::cout << "Calling non-exported method...\n";
    InnerMethod();
}

void InnerMethod()
{
    std::cout << "No export, not visible from other modules.\n";
}