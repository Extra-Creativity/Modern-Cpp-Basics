#include "a.h"

int a = 1; // defines extern int a in a.h.
// define extern const int b in b.h; extern is necessary.
extern const int b = 10086;

// internal linkage below.
[[maybe_unused]] static int c = 1;
static void Func(); // declarations
namespace {

void Func2(); // declarations

}

[[maybe_unused]] static void Func(){
    // Cannot be referenced by other TUs.
}

namespace{

[[maybe_unused]] void Func2(){
    // Cannot be referenced by other TUs.
}

}