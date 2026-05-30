add_rules("mode.debug", "mode.release")
add_requires("wayland-client", "wayland-protocols", { system = true })
set_languages("c++26")

rule("wayland.protocol")
    set_extensions(".xml")
    on_load(function (target)
        local gen_dir = path.join(target:autogendir(), "rules", "wayland")
        if not os.isdir(gen_dir) then
            os.mkdir(gen_dir)
        end
        target:add("includedirs", gen_dir, {public = true})
        for _, srcfile in ipairs(target:sourcefiles()) do
            if srcfile:endswith(".xml") then
                local prot_name = path.basename(srcfile)
                local sourcefile_c = path.join(gendir, prot_name .. "-protocol.c")
                target:add("files", sourcefile_c)
            end
        end
    end)
    before_build_file(function (target, srcfile, opts)
        import("core.project.depend")
        import("core.tool.compiler")
        local prot_name = path.basename(srcfile)
        local gendir = path.join(target:autogendir(), "rules", "wayland")
        local headerfile = path.join(gendir, prot_name .. "-client-protocol.h")
        local sourcefile_c = path.join(gendir, prot_name .. "-protocol.c")
        depend.on_changed(function ()
            os.runv("wayland-scanner", {"client-header", srcfile, headerfile})
            os.runv("wayland-scanner", {"private-code", srcfile, sourcefile_c})
        end, { files = srcfile})

        target:add("files", sourcefile_c)
    end)
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
    add_packages("wayland", "wayland-protocols")
