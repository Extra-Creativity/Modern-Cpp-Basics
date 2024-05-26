set_languages("cxxlatest")
add_rules("mode.debug", "mode.release")

target("optional")
    add_files("optional.cpp")

target("expected")
    add_files("expected.cpp")
