#pragma once
#include <cstdint>

#ifdef NEED_PARAM
void SomeOldLibFunc(std::uint32_t);
#else
void SomeOldLibFunc(std::uint32_t = 1);
#endif