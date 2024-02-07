# 02-Basics

### 上半部分

1. 使用gcc（善用compiler explorer）编译并运行下面的程序：

   ```c++
   #include <limits>
   int main()
   {
       int a = std::numeric_limits<int>::max();
       int b = a + 1;
       return 0;
   }
   ```

   随后尝试加上编译选项`-ftrapv`，看看运行结果有没有变化。

2. 打印一个`std::uint8_t`的变量的地址；如果你想用`print`，可以使用`std::println("{}", 你的地址)`。

3. 在2001年的游戏《雷神之锤3》（Quake III Arena）中，使用了一种快速倒数平方根算法，用于对于32位浮点数`x`计算$1/\sqrt x$的近似值。它的算法如下：

   ```pseudocode
   /* 设操作BitEquiv表示两个数在二进制表示上相同, i32表示32位整数，f32表示32位浮点数。*/
   f32 GetInvSqrt(f32 num)
   {
   	i32 a <- BitEquiv(num);
   	i32 b <- 0x5f3759df - (a >> 1);
   	f32 y <- BitEquiv(b);
   	return y * (1.5f - (num * 0.5f * y * y));
   }
   ```

   试把上述伪代码转为C++代码。可以尝试几个数，看看和`<cmath>`中的`std::sqrtf`相比的误差；如果你感兴趣，也可以在quick-bench上比一比性能。

   > 当然，这个算法在目前是比CPU硬件指令更慢的，只是在2000年代更好。它本质上使用了牛顿迭代法，`return`的步骤进行了一次迭代，如果想要更高的精度可以把它赋给`y`，继续用该式迭代。

4. 写一个将二进制数据转为HTTP要求的字节序的函数。注意用`std::endian`区分当前机器大小端的情况；如果两个都不是，打印一个警告。

5. 判断以下程序的合法性：

   ```c++
   int a[][3] = { {1, 2, 3}, {4,5,6} };
   int (*b)[3] = a; // 合法吗？
   int **c = a; // 合法吗？
   int **d = b; // 合法吗？
   ```

   ```c++
   int a[]{1,2,3}, b[]{4,5,6,7};
   void Func(int (&a)[3]) { /* ... */}
   void Func2(int a[3]) { /* ... */}
   
   Func(a); Func2(a);
   Func(b); Func2(b); // 这四个里面哪个合法？
   ```

   希望你可以通过这个例子区分指针和数组，加深对数组decay的理解；指针所指向的目标不会继续decay，引用所引用的目标也会暂时保持原类型（不过仍然允许后续的decay，比如上面的`Func`中`int* p = a;`是合法的）。

6. 写一个函数，它接受一个返回值为`int`、参数为`float`和`double`的函数指针为参数，返回`void`。

7. 写一个scoped enumeration，它包含`read, write, exec`三个选项；同时为了使它们可以按位组合，编写一个配套的重载的按位与和按位或的运算符。为了一般性，可以使用`auto b = std::to_underlying()`；如果你的编译器不支持，可以使用`using T = std::underlying_type<Enum>`，再手动转为`T`。

8. 对于下面的程序：

   ```c++
   #include <map> 
   int main()
   { 
   	std::map<int, int> m; 
   	m[0] = m.size();
   }
   ```

   按照C++17标准规定，`m[0]`是什么？