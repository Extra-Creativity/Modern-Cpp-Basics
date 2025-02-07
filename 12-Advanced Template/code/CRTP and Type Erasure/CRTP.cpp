#include <print>

template<typename Derived>
class StudentBase
{
public:
    using Super = StudentBase<Derived>;
    float GetGPA()
    {
        return static_cast<Derived *>(this)->GetGPACoeff() * 4.0f;
    }
};

class Student : public StudentBase<Student>
{
    friend Super;
    float GetGPACoeff() { return 0.8f; }
};

class JuanWang : public StudentBase<Student>
{
    friend Super;
    float GetGPACoeff() { return 1.0f; }
};

template<typename T>
void PrintGPA(StudentBase<T> &student)
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