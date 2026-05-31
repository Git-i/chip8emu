add_rules("mode.debug", "mode.release")
add_requires("wayland-client", "wayland-protocols", "xkbcommon", { system = true })
set_languages("c++26")

target("wayland-xdg-protocol")
    set_kind("object")
    on_config(function (target)
        import("core.project.depend")
        local pkgdatadir, _ = os.iorun("pkg-config --variable=pkgdatadir wayland-protocols")
        pkgdatadir = pkgdatadir:trim()
        local srcfile = pkgdatadir .. "/stable/xdg-shell/xdg-shell.xml"
        local gendir = path.join(target:autogendir(), "rules", "wayland")
        if not os.isdir(gendir) then
            os.mkdir(gendir)
        end
        local headerfile = path.join(gendir, "xdg-shell-client-protocol.h")
        local sourcefile_c = path.join(gendir, "xdg-shell-protocol.c")
        -- depend.on_changed(function ()
        os.runv("wayland-scanner", {"client-header", srcfile, headerfile})
        os.runv("wayland-scanner", {"private-code", srcfile, sourcefile_c})
        -- end, { files = srcfile})
        target:add("includedirs", gendir, {public=true})
        target:add("files", sourcefile_c)
    end)


target("chip8")
    set_kind("binary")
    add_deps("wayland-xdg-protocol")
    add_files("src/*.cpp")
    add_files("src/*.cppm")
    set_warnings("allextra", "error")
    set_policy("build.c++.modules", true)
    add_packages("wayland", "wayland-protocols", "xkbcommon")
