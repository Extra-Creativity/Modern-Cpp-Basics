#include <format>
#include <iostream>
#include <string>

// 大端法表示
template<typename T>
void DecomposeToByte(const T& str)
{
    for (int i = 0; i < str.size(); i++)
    {
        auto ch = static_cast<unsigned int>(str[i]);
        unsigned int mask = (1 << CHAR_BIT) - 1;
        for (int j = 0; j < sizeof(str[0]); j++)
        {
            std::cout << std::format("{:02x} ", (mask & ch));
            ch >>= 8;
        }
    }
    std::cout << "\n";
    return;
}

int main()
{
    std::wstring s1 = L"\U0000FFFF"; DecomposeToByte(s1);
    std::u8string s2 = u8"\U0001F449"; DecomposeToByte(s2);
    std::u16string s3 = u"\U0001F449"; DecomposeToByte(s3);
    std::u32string s4 = U"\U0001F449"; DecomposeToByte(s4);
    return 0;
}