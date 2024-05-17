#pragma once
class Singleton
{
public:
    static Singleton& GetInstance();
private:
    Singleton() = default;
};