#include "singleton.h"

Singleton& Singleton::GetInstance(){
    static Singleton* instance = new Singleton{};
    return *instance;
}