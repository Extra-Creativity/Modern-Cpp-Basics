template<int N> // This should be std::size_t at best; we just want to
                // illustrate lazy instantiation, so here we need N <= 0
                // to make Array<N> ill-formed.
class Array
{
public:
    int arr[N];
};

template<typename T>
T v = T::default_value();

template<typename T, int N>
class A
{
public:
    using M = T::value_type;

    void Func(T a = T{}) {}

    void error() { Array<-1> boom; }

    // Successful ones
    void Test() { Array<N> arr; }
    struct Test2
    {
        Array<N> arr;
    };

    void Test4(Array<N> *arr) {}

    // Failed ones
    void Test3(Array<N> arr) {}
    union {
        Array<N> arr;
        int m;
    };
};

int main()
{
    A<int, -1> a;
}