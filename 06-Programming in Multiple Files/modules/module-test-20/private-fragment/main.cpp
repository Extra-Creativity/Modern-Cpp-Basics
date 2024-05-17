#include <iostream>
import Person;

int main()
{
    std::cout << "---------module-private---------\n";
    auto p = CreatePerson();
    // p->Print(); // error: use undefined type "Person"
    PrintPerson(p);
    DestroyPerson(p);
    return 0;
}