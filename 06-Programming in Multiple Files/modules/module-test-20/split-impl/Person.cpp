module;
#include <iostream>
#include <cstdint>
#include <format>
#include <string>
module Person; // denote that it's a module implementation

Person::Person(const std::string& init_name, std::uint64_t init_id):
    name{init_name}, id{init_id} {}

void Person::Print() const
{
    std::cout << std::format("Person #{}: {}\n", id, name);
}
