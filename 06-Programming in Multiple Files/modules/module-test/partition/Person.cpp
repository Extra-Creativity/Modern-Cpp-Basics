module Person; // denote that it's a module implementation
import std;

Person::Person(const std::string& init_name, std::uint64_t init_id):
    name{init_name}, id{init_id} {}

void Person::Print() const
{
    std::println("Person #{}: {}", id, name);
}
