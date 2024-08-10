set_languages("cxxlatest")
set_policy("build.warning", true)
set_warnings("all")

set_encodings("utf-8")

add_rules("mode.debug", "mode.release")

target("TCPStream")
    set_kind("static")
    add_files("src/Socket.cpp")

target("server")
    add_deps("TCPStream")
    add_files("src/server.cpp")

target("client")
    add_deps("TCPStream")
    add_files("src/client.cpp")