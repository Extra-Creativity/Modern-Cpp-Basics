#include <iostream>
#include "Vector3.h"

void Hello(){
    std::cout << "Hello, world!\n";
}

void Hello2(Vector3 vec){
    std::cout << vec.GetLength();
}