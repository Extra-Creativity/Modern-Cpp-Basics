set_project("Programming in Multiple Files")
set_xmakever("2.8.7")
set_version("0.0.0")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})
set_languages("cxx23")
set_policy("build.warning", true)
set_warnings("all")
set_encodings("utf-8")

target("example")
    set_kind("binary")
    add_headerfiles("example1/*.h")
    add_files("example1/*.cpp")

target("inline-example")
    set_kind("binary")
    add_headerfiles("inline-example/*.h")
    add_files("inline-example/*.cpp")
    
target("linkage-test")
    set_kind("binary")
    add_headerfiles("linkage-test/*.h")
    add_files("linkage-test/*.cpp")

includes("inline-test", "extern-C-test")
