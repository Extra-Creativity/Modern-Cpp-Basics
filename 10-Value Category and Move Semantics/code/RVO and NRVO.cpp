#include <iostream>
class Object
{
public:
    Object() { std::cout << "Construct at " << this << "\n"; };
    ~Object() { std::cout << "Destruct at " << this << "\n"; };
    Object(const Object&) { 
        std::cout << "Const Copy at " << this << "\n"; 
    };
    Object(Object&&) { std::cout << "Move at " << this << "\n"; };
    Object& operator=(const Object&) {
        std::cout << "Const Copy Assignment at " << this << "\n";
        return *this;
    };
    Object& operator=(Object&&) {
        std::cout << "Move Assignment at " << this << "\n";
        return *this;
    };
};

Object GetObject_RVO()
{
    return Object();
}

Object GetObject_NRVO()
{
    Object obj;
    return obj;
}

int main()
{
    std::cout << std::hex;

    std::cout << "RVO\n";
    Object obj1;
    obj1 = GetObject_RVO();

    std::cout << "\nNRVO\n";
    Object obj2;
    obj2 = GetObject_NRVO();

    std::cout << "\nDone.\n";
    return 0;
}