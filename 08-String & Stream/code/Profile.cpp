//********************************************************
// The following code example is taken from the book
//  C++20 - The Complete Guide
//  by Nicolai M. Josuttis (www.josuttis.com)
//  http://www.cppstd20.com
//
// The code is licensed under a
//  Creative Commons Attribution 4.0 International License
//  http://creativecommons.org/licenses/by/4.0/
//********************************************************

#include <charconv>
#include <chrono>
#include <format>
#include <iostream>
#include <sstream>
#include <string>


static void checkSPrintf(int num)
{
    for (int idx = 0; idx < num; ++idx)
    {
        int i = 42;
        double d = 7.7;
        char buf[100] = { '?', '?', '?', '?', '?', '?', '?',
                          '?', '?', '?', '?', '?', '?' };
        sprintf(buf, "%d %f", i, d);
        if (num == 1)
        {
            std::cout << buf << '\n';
            return;
        }
    }
}

static void checkOStringStream(int num)
{
    for (int idx = 0; idx < num; ++idx)
    {
        int i = 42;
        double d = 7.7;
        std::ostringstream os;
        os << i << ' ' << d;
        if (num == 1)
        {
            std::cout << os.str() << '\n';
            return;
        }
    }
}

static void checkToString(int num)
{
    for (int idx = 0; idx < num; ++idx)
    {
        int i = 42;
        double d = 7.7;
        std::string s = std::to_string(i) + ' ' + std::to_string(d);
        if (num == 1)
        {
            std::cout << s << '\n';
            return;
        }
    }
}

static void checkToChars(int num)
{
    for (int idx = 0; idx < num; ++idx)
    {
        int i = 42;
        double d = 7.7;
        char buf[100] = { '?', '?', '?', '?', '?', '?', '?',
                          '?', '?', '?', '?', '?', '?' };
        std::to_chars_result res = std::to_chars(buf, buf + 9, i);
        *res.ptr = ' ';
        res = std::to_chars(res.ptr + 1, res.ptr + 10, d);
        *res.ptr = '\0';
        if (num == 1)
        {
            std::cout << buf << '\n';
            return;
        }
    }
}

static void checkFormat(int num)
{
    for (int idx = 0; idx < num; ++idx)
    {
        int i = 42;
        double d = 7.7;
        auto s = std::format("{} {}", i, d);
        if (num == 1)
        {
            std::cout << s << '\n';
            return;
        }
    }
}

static void checkFormatTo(int num)
{
    for (int idx = 0; idx < num; ++idx)
    {
        int i = 42;
        double d = 7.7;
        char buf[100] = { '?', '?', '?', '?', '?', '?', '?',
                          '?', '?', '?', '?', '?', '?' };
        auto ret = std::format_to_n(buf, 99, "{} {}", i, d);
        *(ret.out) = '\0';
        if (num == 1)
        {
            std::cout << buf << '\n';
            return;
        }
    }
}

template<typename T>
void measure(std::string s, T func)
{
    auto t0 = std::chrono::steady_clock::now();
    func(1000); // measure
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff{ t1 - t0 };
    std::cout << s << ": " << diff.count() << "ms\n ";
    func(1); // show output
}

int main()
{
    measure("sprintf()    ", checkSPrintf);
    measure("ostringstream", checkOStringStream);
    measure("to_string()  ", checkToString);
    measure("to_chars()   ", checkToChars);
    measure("format()     ", checkFormat);
    measure("format_to()  ", checkFormatTo);
    return 0;
}
