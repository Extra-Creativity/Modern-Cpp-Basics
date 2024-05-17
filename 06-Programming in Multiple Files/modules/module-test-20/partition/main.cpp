#include <iostream>
import Person;

int main()
{
    std::cout << "---------module-partition---------\n";
    Person person{"Haoyang", 6};
    person.Print();
    PrintOrder();
    return 0;
}