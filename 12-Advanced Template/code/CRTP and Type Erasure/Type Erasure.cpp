#include <exception>
#include <print>
#include <utility>

class StudentProxyBase
{
public:
    virtual float GetGPACoeff() = 0;
    virtual StudentProxyBase *clone() const = 0;
    virtual ~StudentProxyBase() = default;
};

template<typename ConcreteStudentType>
class StudentProxy : public StudentProxyBase
{
    ConcreteStudentType student;

public:
    // You can also add another overload for ConcreteStudentType&&.
    StudentProxy(const ConcreteStudentType &init_student)
        : student{ init_student }
    {
    }
    float GetGPACoeff() override { return student.GetGPACoeff(); }
    StudentProxy *clone() const override { return new StudentProxy{ student }; }
};

class StudentBase
{
    StudentProxyBase *studentProxy = nullptr;

public:
    StudentBase() = default;

    template<typename T>
    StudentBase(T &&student)
    {
        studentProxy = new StudentProxy{ std::forward<T>(student) };
    }

    StudentBase(const StudentBase &another)
        : studentProxy{ another.studentProxy->clone() }
    {
    }

    StudentBase(StudentBase &&another)
        : studentProxy{ std::exchange(another.studentProxy, nullptr) }
    {
    }

    float GetGPA()
    {
        if (studentProxy == nullptr)
        {
            throw std::runtime_error{
                "Cannot deference without underlying student"
            };
        }
        return studentProxy->GetGPACoeff() * 4.0f;
    }
    ~StudentBase() { delete studentProxy; }
};

class Student
{
public:
    float GetGPACoeff() { return 0.8f; }
};

class JuanWang
{
public:
    float GetGPACoeff() { return 1.0f; }
};

void PrintGPA(StudentBase student)
{
    std::println("{}", student.GetGPA());
}

int main()
{
    PrintGPA(Student{});
    PrintGPA(JuanWang{});
}