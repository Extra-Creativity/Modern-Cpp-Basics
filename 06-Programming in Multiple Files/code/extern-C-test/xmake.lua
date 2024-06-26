target("extern-C-lib1")
    set_kind("static")
    add_headerfiles("test1/a.h")
    add_files("test1/a.cpp")

target("extern-C-test1")
    set_kind("binary")
    add_deps("extern-C-lib1")
    add_headerfiles("test1/a.h")
    add_files("test1/main.c")

target("extern-C-lib2")
    set_kind("static")
    add_headerfiles("test2/a.hpp")
    add_files("test2/a.cpp")

target("extern-C-test2")
    set_kind("binary")
    add_deps("extern-C-lib2")
    add_headerfiles("test2/a.h")
    add_files("test2/main.c")
