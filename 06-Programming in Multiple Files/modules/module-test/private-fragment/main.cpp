import std;
import Person;

int main()
{
    std::println("---------module-private---------");
    auto p = CreatePerson();
    // p->Print(); // error: use undefined type "Person"
    PrintPerson(p);
    DestroyPerson(p);
    return 0;
}