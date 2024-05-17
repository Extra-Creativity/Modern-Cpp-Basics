#pragma once
// declares the template
template<typename T> void Func(const T&);

class A
{
public:
    template<typename T> void Func(T); // declaration
};

template<typename T>
void A::Func(T x){ /* ... */} // definition

template<typename T>
class B
{
public:
    template<typename U> void Func(U);    
};

template<typename T>
    template<typename U>
void B<T>::Func(U){ /* ... */}
