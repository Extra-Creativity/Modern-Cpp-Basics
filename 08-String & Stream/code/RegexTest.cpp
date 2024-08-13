#include "ctre.hpp"
#include "re2/re2.h"
#include <print>

// 注意RE2有一段额外的括号，因为不会默认进行全局的捕获。
static constexpr re2::StringPiece re2Regex{ R"(((\d+)\.(\d+)\.(\d+)\.(\d+)))" };
static constexpr CTRE_REGEX_INPUT_TYPE ctreRegex{
    R"((\d+)\.(\d+)\.(\d+)\.(\d+))"
};
static const std::string singleStr = "127.0.0.1";
static const std::string multiStr = "Test: 127.0.0.1; Test2: 127.0.0.2aaa";

void MatchTest()
{
    re2::StringPiece whole1;
    int a1, b1, c1, d1;
    if (RE2::FullMatch(singleStr, re2Regex, &whole1, &a1, &b1, &c1, &d1))
    {
        std::println("re2 Pass: {} {} {} {} {}", whole1, a1, b1, c1, d1);
    }
    else
    {
        std::println("Fail");
    }

    // Notice: structured binding in if clause is not regulated in the standard
    // So the second whole2 is necessary. MSVC rejects it if it's not provided.
    if (auto [whole2, a2, b2, c2, d2] = ctre::match<ctreRegex>(singleStr);
        whole2)
    {
        std::println("ctre Pass: {} {} {} {} {}", whole2.to_view(),
                     a2.to_number(), b2.to_number(), c2.to_number(),
                     d2.to_number());
    }
    else
    {
        std::println("Fail");
    }
}

void SearchTest()
{
    re2::StringPiece whole;
    if (RE2::PartialMatch(multiStr, re2Regex, &whole))
    {
        std::println("re2 Pass: {}", whole);
    }
    else
    {
        std::println("Fail");
    }

    if (auto m = ctre::search<ctreRegex>(singleStr))
    {
        std::println("ctre Pass: {}", m.to_view());
    }
    else
    {
        std::println("Fail");
    }
}

void SearchAllTest()
{
    re2::StringPiece whole, multiStrView{ multiStr };
    while (RE2::FindAndConsume(&multiStrView, re2Regex, &whole))
    {
        std::println("re2 Pass: {}", whole);
    }

    for (auto m : ctre::search_all<ctreRegex>(multiStr))
    {
        std::println("ctre Pass: {}", m.to_view());
    }
}

// re2 only
void ReplaceTest()
{
    std::string newString{ multiStr };
    // 用第二个捕获（127）进行替换
    if (RE2::Replace(&newString, re2Regex, "\\2"))
        std::println("re2 Replace test: {}", newString);
    newString = multiStr;
    if (RE2::GlobalReplace(&newString, re2Regex, "SOME_IP\\2"))
        std::println("re2 Global replace test: {}", newString);
}

// ctre only
void TokenizeTest()
{
    for (auto token : ctre::tokenize<R"(\d+\.)">("127.0.0.1"))
        std::print("ctre tokenize: {} ", token.to_view());
    std::print("\n");
    for (auto m : ctre::split<ctreRegex>(multiStr))
        std::println("ctre split: {} {}", m.to_view(), m.get<1>().to_view());
}

int main()
{
    MatchTest();
    SearchTest();
    SearchAllTest();
    ReplaceTest();
    TokenizeTest();
    return 0;
}