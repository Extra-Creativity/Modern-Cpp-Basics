#include <iostream> // This is not standard, but we don't want main to be a module.
                    // So this clang dialect is adopted.
import Customer; // Customer has "export import Person", 
                 // so all exported things of Person can be used.

int main()
{
    std::cout << "---------module-simple---------\n";
    HelloWorld();
    TemplateMethod<int>();
    // InnerMethod(); // error: cannot find identifier (because it's not exported)
    
    Person person{"Haoyang", 6};
    person.Print();
    Customer customer{ "lrzzzzzzzzzz", 666 };
    customer.Print();
    return 0;
}