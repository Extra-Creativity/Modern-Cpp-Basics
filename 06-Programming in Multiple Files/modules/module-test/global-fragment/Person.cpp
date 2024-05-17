module; // global module fragment

#define NEED_PARAM
#include "Old.h"

module Person; // denote that it's a module implementation
import std;

Person::Person(const std::string& init_name, std::uint64_t init_id):
    name{init_name}, id{init_id} {}

void Person::Print() const
{
    std::println("Person #{}: {}", id, name);
}

void InnerMethod();

// No need to specify any export; it's the duty of module interface!
void HelloWorld()
{
    SomeOldLibFunc(1);
    std::println("Hello, world!");
    std::println("Calling non-exported method...");
    InnerMethod();
}

void InnerMethod()
{
    std::println("No export, not visible from other modules.");
}