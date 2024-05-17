#pragma once

#if defined FUNC_H_IMPORT_
#define DLL_MACRO_NEED_IMPORT
#elif defined FUNC_H_EXPORT_
#define DLL_MACRO_NEED_EXPORT
#endif

#include "DLLMacro.h"

DLL_PORT void Func();
class DLL_PORT A {};

#undef DLL_PORT