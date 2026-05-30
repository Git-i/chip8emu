#include <sys/mman.h>
#include <wayland-client-protocol.h>
#include "wayland-client.h"
#include "xdg-shell-client-protocol.h"
import std;
import chip8;
import wayland_wrapper;

using namespace std;
using namespace std::literals;

struct State {
    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    xdg_wm_base* wm_base = nullptr;
    wl_surface* surface = nullptr;
    xdg_surface* xdg_surf = nullptr;
    xdg_toplevel* top_level = nullptr;
    wayland::UniqueHandle shm_file{-1};
    wl_shm_pool* shm_pool = nullptr;
    wayland::MMappedPointer shm_pool_ptr{nullptr, 0};
    array<pair<wl_buffer*, bool>, 2> buffers;
    size_t shm_pool_size = 0;
    const uint32_t display_scale = 10;
    
    chip8::Machine chip8_mch;

    static constexpr auto min_width = chip8::Display::width;
    static constexpr auto min_height = chip8::Display::height;
    static constexpr auto bpp = 4u;

    State(const filesystem::path& path)
        : chip8_mch(chip8::Machine::from_file(path)) {
    }

    void registry_global(wl_registry* reg, uint32_t name, string_view interface, uint32_t version) {
        std::ignore = reg;
        if (interface == wl_compositor_interface.name) {
            compositor = static_cast<wl_compositor*>(
                wl_registry_bind(reg, name, &wl_compositor_interface, 4));
        } else if (interface == wl_shm_interface.name) {
            shm = static_cast<wl_shm*>(
                wl_registry_bind(reg, name, &wl_shm_interface, 1));
        } else if (interface == xdg_wm_base_interface.name) {
            wm_base = static_cast<xdg_wm_base*>(
                wl_registry_bind(reg, name, &xdg_wm_base_interface, 1));
        }
        println("interface: {}, version: {}, name: {}", interface, version, name);
    }
    void xdg_wm_base_ping(xdg_wm_base* base, uint32_t serial) {
        xdg_wm_base_pong(base, serial);
    }
    void buffer_release(wl_buffer* buffer) {
        auto it = ranges::find(buffers, buffer, &decltype(buffers)::value_type::first);
        it->second = true;
    }
    void xdg_surface_configure(xdg_surface* surf, uint32_t serial) {
        xdg_surface_ack_configure(surf, serial);
        wl_surface_attach(surface, buffers[0].first, 0, 0);
        wl_surface_commit(surface);
        buffers[0].second = false;
    }
    void create_surfaces() {
        surface = wl_compositor_create_surface(compositor);
        xdg_surf = xdg_wm_base_get_xdg_surface(wm_base, surface);
        top_level = xdg_surface_get_toplevel(xdg_surf);
        xdg_toplevel_set_title(top_level, "Chip8 EMU");
        wayland::add_xdg_wm_base_listener(wm_base, *this);
        wayland::add_xdg_surface_listener(xdg_surf, *this);
    }
    void registry_global_remove(wl_registry* reg, uint32_t name) {
        std::ignore = reg; std::ignore = name;
    }
    void create_and_map_shm_pool(size_t pool_size) {
        shm_pool_size = pool_size;
        shm_file = wayland::create_shm_file()
            .and_then(bind_back<wayland::allocate_shm_file>(shm_pool_size))
            .value();
        shm_pool = wl_shm_create_pool(shm, shm_file.get(), shm_pool_size);
        shm_pool_ptr = wayland::MMappedPointer{
            mmap(nullptr, shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_file.get(), 0),
            shm_pool_size
        };
    }
    constexpr auto display_width() const -> uint32_t { return min_width * display_scale; }
    constexpr auto display_height() const -> uint32_t { return min_height * display_scale; }
    constexpr auto get_pool_size() const -> size_t {
        return display_width() * display_height() * bpp * 2;
    }
    void create_buffers() {
        buffers[0] = { wl_shm_pool_create_buffer(shm_pool, 0, display_width(), display_height(), display_width() * bpp,
                        WL_SHM_FORMAT_XRGB8888), true };
        buffers[1] = { wl_shm_pool_create_buffer(shm_pool, shm_pool_size / 2, display_width(), display_height(), display_width() * bpp,
                        WL_SHM_FORMAT_XRGB8888), true };
        for (auto [buf, is_valid] : buffers)
            wayland::add_buffer_listener(buf, *this);
    };
    void callback_done(wl_callback* cb, uint32_t) {
        wl_callback_destroy(cb);
        cb = wl_surface_frame(surface);
        wayland::add_callback_listener(cb, *this);
        // chip8_mch.step();
        // write into the offset of the first free buffer
        const auto it = ranges::find(buffers, true, &decltype(buffers)::value_type::second);
        const auto off = ranges::distance(buffers.begin(), it);

        auto dst_pointer = reinterpret_cast<uint32_t*>(shm_pool_ptr.get<std::byte>() + (off * shm_pool_size / 2));
        blit_frame(
             chip8_mch.display.get_pixels(), 
             mdspan<uint32_t, dextents<uint32_t, 2>>(dst_pointer, display_height(), display_width()));
        wl_surface_attach(surface, it->first, 0, 0);
        wl_surface_damage_buffer(surface, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(surface);
    }
    void blit_frame(const mdspan<uint8_t, chip8::Display::Extents> src, mdspan<uint32_t, dextents<uint32_t, 2>> dst) {
        const auto r8_to_color = [](uint8_t in) -> uint32_t {
            const uint32_t as_u32 = in;
            return 
                as_u32 | (as_u32 << 8) | (as_u32 << 16);
        };
        // copy src into dst with nearest neighbor filtering
        for (const auto i : views::iota(0u, src.extent(0))) {
            for (const auto j : views::iota(0u, src.extent(1))) {

                for (const auto inter_i : views::iota(0u, display_scale))
                    for (const auto inter_j : views::iota(0u, display_scale))
                     dst[
                        i * display_scale + inter_i,
                        j * display_scale + inter_j
                     ] = r8_to_color(src[i, j]);

            }
        }
    }
    void initial_draw_callback() {
        auto frame = wl_surface_frame(surface);
        wayland::add_callback_listener(frame, *this);
    }
};

int main() {
    State wl_state(filesystem::path() / "roms" / "octojam2title.ch8");
    auto display = wayland::Display::connect().value();
    auto registry = display.get_registry();
    registry.add_listener(wl_state);
    display.roundtrip();
    wl_state.create_and_map_shm_pool(wl_state.get_pool_size()); 
    wl_state.create_buffers();
    wl_state.create_surfaces();
    wl_surface_commit(wl_state.surface);

    wl_state.initial_draw_callback();
    while (display.dispatch()) { 
        for(auto _ : views::iota(0u, 30u)) wl_state.chip8_mch.step(); 
    }
}
