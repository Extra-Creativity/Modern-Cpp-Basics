# Answer

1. ```c++
   template<typename T>
   T exchange(T& a, const T& b)
   {
       T temp{ std::move(a) };
       a = b;
       return temp; // NRVO
   }
   ```

2. 显然不可以，因为用值类型替换重载的前提是已经定义好了相应的构造函数；如果我们进行这种替换，则会出现递归调用。

   假设构造函数正常编写了；对于赋值函数，我们可以修改一句来使其正确，代码如下：

   ```c++
   class A
   {
   public:
       A& operator=(A obj) {
           swap(*this, obj);
           return *this;
       }
   };
   ```

   这本质上就是不进行自赋值判断的copy-and-swap idiom（当然，自赋值在这个idiom里不会引起致命错误，只会导致不提前返回，运行效率有所下降）。如果有些人这样编写赋值函数也不要太惊讶。