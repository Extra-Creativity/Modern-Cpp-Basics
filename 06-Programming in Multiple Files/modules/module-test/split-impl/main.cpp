import std;
import Person;

int main()
{
    std::println("---------module-split-impl---------");
    HelloWorld();
    TemplateMethod<int>();
    // InnerMethod(); // error: cannot find identifier (because it's not exported)
    Person person{"Haoyang", 6};
    person.Print();
    return 0;
}