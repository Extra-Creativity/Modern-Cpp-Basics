# Assignment

1. 对以下代码片段，使用移动语义进行优化。

   ```c++
   // 将文件按行划分，得到行的数组。
   std::ifstream fin{ "test.txt" };
   std::vector<std::string> allRows;
   std::string row;
   while (std::getline(fin, row)) {
       allRows.push_back(row);
   }
   ```

2. 我们在C语言中是按如下方式对两个整数进行交换的：

   ```c++
   void swap(int* ptr1, int* ptr2)
   {
       int temp = *ptr1;
       *ptr1 = *ptr2;
       *ptr2 = temp;
   }
   ```

   在C++中我们可以通过引用来增加一些可读性，也可以通过模板来定义一般的交换代码。请写出模板`swap`，注意考虑能否在其中使用移动语义。

3. 有一些人提出，如果移动赋值实现为逐成员交换可以在一些情况下里高效率，例如我们擦除一个`vector`的第一个元素，那么事实上内部发生的是：

   ```c++
   for (std::size_t i = 1; i < size; i++)
       ptr[i - 1] = std::move(ptr[i]);
   Destroy(ptr[size - 1]);
   size--;
   ```

   假如我们的类型如下：

   ```c++
   class Foo
   {
       char *ptr = nullptr;
       void ExpensiveDelete()
       {
           std::println("expensive delete"); // 我们假设这里代表了比较昂贵的操作
           delete ptr;
       }
       
   public:
       ~Foo(){ ExpensiveDelete(); }
   
       Foo& operator=(Foo&& another)
       {
           if (this == &another)
               return *this;
   
           ExpensiveDelete();
           ptr = std::exchange(another.ptr, nullptr);
           return *this;
       }
   };
   ```

   对于如下代码：

   ```c++
   std::vector<Foo> vf(10);
   vf.erase(vf.begin());
   ```

   会输出多少次`expensive delete`？如果我们将移动赋值函数改为逐成员交换，那么会输出多少次`expensive delete`？如果你想反驳这个理由，你应该在增加什么来达到相同的效果？

4. 给Error Handling作业中`List`增加移动构造函数和移动赋值函数；再使用copy-and-swap idiom改写。

