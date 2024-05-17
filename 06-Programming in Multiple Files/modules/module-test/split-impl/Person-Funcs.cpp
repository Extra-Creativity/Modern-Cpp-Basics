module Person; // denote that it's a module implementation
import std;

void InnerMethod();

// No need to specify any export; it's the duty of module interface!
void HelloWorld()
{
    std::println("Hello, world!");
    std::println("Calling non-exported method...");
    InnerMethod();
}

void InnerMethod()
{
    std::println("No export, not visible from other modules.");
}