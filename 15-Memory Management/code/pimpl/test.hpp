#pragma once
#include <memory>

class SomeComplexClass
{
    struct Impl;
    std::unique_ptr<Impl> impl_;

public:
    SomeComplexClass(int a, float b);
    SomeComplexClass(const SomeComplexClass &);
    SomeComplexClass &operator=(const SomeComplexClass &);
    SomeComplexClass(SomeComplexClass &&) noexcept;
    SomeComplexClass &operator=(SomeComplexClass &&) noexcept;
    ~SomeComplexClass();
    float Sum() const noexcept;
    float Prod() const noexcept;
};