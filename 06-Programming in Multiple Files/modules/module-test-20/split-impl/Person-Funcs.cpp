module;
#include <iostream>

module Person; // denote that it's a module implementation

void InnerMethod();

// No need to specify any export; it's the duty of module interface!
void HelloWorld()
{
    std::cout << "Hello, world!\n";
    std::cout << "Calling non-exported method...\n";
    InnerMethod();
}

void InnerMethod()
{
    std::cout << "No export, not visible from other modules.\n";
}