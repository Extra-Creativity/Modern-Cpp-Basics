add_rules("use-clang")
set_group("module-example-20")

target("module-simple")
    set_kind("binary")
    -- when no .mpp exists, you need this additional line.
    -- set_policy("build.c++.modules", true)
    add_files("simple/main.cpp")
    add_files("simple/Person.mpp", "simple/Customer.mpp")

target("module-split-impl")
    set_kind("binary")
    add_files("split-impl/*.cpp")
    add_files("split-impl/Person.mpp")

-- target("module-header-unit")
--     set_kind("binary")
--     add_headerfiles("header-unit/*.h")
--     add_files("header-unit/*.cpp")
--     add_files("header-unit/Person.mpp")

target("module-global-fragment")
    set_kind("binary")
    add_headerfiles("global-fragment/*.h")
    add_files("global-fragment/*.cpp")
    add_files("global-fragment/Person.mpp")

target("module-partition")
    set_kind("binary")
    add_files("partition/*.cpp")
    add_files("partition/*.mpp")

target("module-private-fragment")
    set_kind("binary")
    add_files("private-fragment/main.cpp")
    add_files("private-fragment/Person.mpp")
