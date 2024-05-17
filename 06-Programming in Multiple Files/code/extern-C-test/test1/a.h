#pragma once

#define EXTERN_C_BEGIN \
    extern "C"         \
    {
#define EXTERN_C_END }

#ifdef __cplusplus
EXTERN_C_BEGIN
#endif

int a;
void Func();

#ifdef __cplusplus
EXTERN_C_END
#endif

#undef EXTERN_C_BEGIN
#undef EXTERN_C_END
