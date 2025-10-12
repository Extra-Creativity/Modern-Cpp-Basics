#include "test.hpp"

struct SomeComplexClass::Impl
{
    int a;
    float b;

    float InnerProd() const noexcept;
};
// Equiv. to private method InnerProd_
float SomeComplexClass::Impl::InnerProd() const noexcept
{
    return a * b;
}

SomeComplexClass::SomeComplexClass(int a, float b) : impl_{ new Impl{ a, b } }
{
}

SomeComplexClass::SomeComplexClass(const SomeComplexClass &another)
    : impl_{ new Impl{ *another.impl_ } }
{
}

SomeComplexClass &SomeComplexClass::operator=(const SomeComplexClass &another)
{
    *impl_ = *another.impl_;
}
// This noexcept is faked up.
SomeComplexClass::SomeComplexClass(SomeComplexClass &&another) noexcept
    : impl_{ new Impl{ std::move(*another.impl_) } }
{
}
SomeComplexClass &SomeComplexClass::operator=(
    SomeComplexClass &&another) noexcept
{
    *impl_ = std::move(*another.impl_);
}

SomeComplexClass::~SomeComplexClass() = default;

float SomeComplexClass::Sum() const noexcept
{
    return static_cast<float>(impl_->a) + impl_->b;
}

float SomeComplexClass::Prod() const noexcept
{
    return impl_->InnerProd();
}