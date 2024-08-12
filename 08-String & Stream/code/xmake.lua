set_languages("cxxlatest")
set_encodings("utf-8")
set_warnings("all")
set_policy("build.warning", true)
add_rules("mode.debug", "mode.release")

add_requires("re2", "ctre")

target("Decompose")
    add_files("Decompose.cpp")

target("Profile")
    add_files("Profile.cpp")

target("RegexTest")
    add_packages("re2", "ctre")
    add_files("RegexTest.cpp")