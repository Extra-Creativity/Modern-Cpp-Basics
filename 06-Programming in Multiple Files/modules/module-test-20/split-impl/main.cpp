#include <iostream>
import Person;

int main()
{
    std::cout << "---------module-split-impl---------\\n";
    HelloWorld();
    TemplateMethod<int>();
    // InnerMethod(); // error: cannot find identifier (because it's not exported)
    Person person{"Haoyang", 6};
    person.Print();
    return 0;
}