#include <concepts>
#include <print>

class StudentBase
{
public:
    using Super = StudentBase;
    float GetGPA(this auto &&self) { return self.GetGPACoeff() * 4.0f; }
};

class Student : public StudentBase
{
    friend Super;
    float GetGPACoeff() { return 0.8f; }
};

class JuanWang : public StudentBase
{
    friend Super;
    float GetGPACoeff() { return 1.0f; }
};

template<typename T>
requires std::derived_from<T, StudentBase>
void PrintGPA(T &student)
{
    std::println("{}", student.GetGPA());
}

int main()
{
    Student s;
    PrintGPA(s);
    JuanWang s2;
    PrintGPA(s2);
}