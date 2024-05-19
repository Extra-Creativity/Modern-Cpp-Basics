set_project("Modules test")
set_xmakever("2.8.7")
set_version("0.0.0")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})
set_languages("cxx23")
set_policy("build.warning", true)
set_warnings("all")
set_encodings("utf-8")

set_toolchains("clang-18")
set_runtimes("c++_shared")
includes("module-test-20")
-- includes("module-test") -- 需要测试import std就用这个，否则用上面这个
