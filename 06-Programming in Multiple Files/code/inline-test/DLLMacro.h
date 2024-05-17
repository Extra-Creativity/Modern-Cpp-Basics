#pragma once
#if defined _WIN32 || defined __CYGWIN__
  #define DLL_IMPORT __declspec(dllimport)
  #define DLL_EXPORT __declspec(dllexport)
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_IMPORT __attribute__ ((visibility ("default")))
    #define DLL_EXPORT __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_IMPORT
    #define DLL_EXPORT
    #define DLL_LOCAL
  #endif
#endif

#if defined DLL_MACRO_NEED_IMPORT
#define DLL_PORT DLL_IMPORT
#elif defined DLL_MACRO_NEED_EXPORT
#define DLL_PORT DLL_EXPORT
#else
#define DLL_PORT
#endif

#undef DLL_MACRO_NEED_IMPORT
#undef DLL_MACRO_NEED_EXPORT
