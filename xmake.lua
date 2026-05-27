add_rules("mode.debug", "mode.release")

set_languages("c++26")

target("chip8")
    set_kind("binary")
    add_files("src/*.cpp")
    add_files("src/*.cppm")
    set_warnings("allextra", "error")
    set_policy("build.c++.modules", true)

