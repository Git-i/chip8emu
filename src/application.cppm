module;
#include <sys/mman.h>
#include <wayland-client-protocol.h>
#include "wayland-client.h"
#include "xdg-shell-client-protocol.h"
export module app;
import wayland_wrapper;
import chip8;
import std;

using namespace std;
export namespace app {
struct State;
// Manages the memory and object required to draw to the screen
struct SurfaceManager {
    State& state;
    uint32_t display_scale = 10;
    xdg_surface* xdg_surf = nullptr;
    wl_surface* surface = nullptr;
    xdg_toplevel* top_level = nullptr;
    array<pair<wl_buffer*, bool>, 2> buffers{};
    wl_shm* shm = nullptr;
    xdg_wm_base* wm_base = nullptr;
    wl_shm_pool* shm_pool = nullptr;
    wayland::UniqueHandle shm_file{-1};
    wayland::MMappedPointer shm_pool_ptr{nullptr, 0};
    size_t shm_pool_size = 0;

    static constexpr auto min_width = chip8::Display::width;
    static constexpr auto min_height = chip8::Display::height;
    static constexpr auto bpp = 4u;

    void create_surfaces();
    void create_buffers();
    void create_and_map_shm_pool(size_t);

    // xdg wm base listener
    void xdg_wm_base_ping(xdg_wm_base* base, uint32_t serial);
    //xdg surface listener
    void xdg_surface_configure(xdg_surface* surf, uint32_t serial); 
    //buffer listener
    void buffer_release(wl_buffer*);
    // callback listener
    void callback_done(wl_callback*, uint32_t);

    constexpr auto display_width() const -> uint32_t { return min_width * display_scale; }
    constexpr auto display_height() const -> uint32_t { return min_height * display_scale; }
    constexpr auto get_pool_size() const -> size_t {
        return display_width() * display_height() * bpp * 2;
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


struct State {
    chip8::Machine chip8_mch;
    wayland::Display display;
    wayland::Registry registry;
    wl_compositor* compositor = nullptr;
    SurfaceManager surface_mgr;
    wl_seat* seat = nullptr;
    wl_keyboard* keyboard = nullptr;


    State(const filesystem::path& path)
        : chip8_mch(chip8::Machine::from_file(path)),
        display(wayland::Display::connect().value()),
        registry(display.get_registry()),
        surface_mgr{.state = *this}
    {
        registry.add_listener(*this);
        display.roundtrip();
        surface_mgr.create_surfaces();
        prepare_input();
        wl_surface_commit(surface_mgr.surface);

        surface_mgr.initial_draw_callback();
    }
    State(const State&) = delete;
    State(State&&) = delete;
    ~State() {
        [[likely]] if (keyboard) wl_keyboard_release(keyboard);
    }
    
    auto dispatch() -> int {
        return display.dispatch();
    }
    void registry_global(wl_registry* reg, uint32_t name, string_view interface, uint32_t version) {
        auto bind_global = [=](auto& target, const wl_interface* intf, uint32_t version) {
            if (interface == intf->name)
                target = static_cast<decltype(auto(target))>(
                    wl_registry_bind(reg, name, intf, version));
        };
        bind_global(compositor, &wl_compositor_interface, 4);
        bind_global(surface_mgr.shm, &wl_shm_interface, 1);
        bind_global(surface_mgr.wm_base, &xdg_wm_base_interface, 1);
        bind_global(seat, &wl_seat_interface, 7);
        println("interface: {}, version: {}, name: {}", interface, version, name);
    }
    void seat_capabilities(wl_seat* seat, uint32_t caps) {
        [[unlikely]] if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
            println(std::cerr, "Keyboard device not found, emulator will function without input");
            return;
        }
        keyboard = wl_seat_get_keyboard(seat);
    }
    void seat_name(wl_seat*, string_view) {}
    void prepare_input() {
        wayland::add_seat_listener(seat, *this);
    }
    void registry_global_remove(wl_registry* reg, uint32_t name) {
        std::ignore = reg; std::ignore = name;
    }
};

void SurfaceManager::create_surfaces() {
    create_and_map_shm_pool(get_pool_size()); 
    create_buffers();

    surface = wl_compositor_create_surface(state.compositor);
    xdg_surf = xdg_wm_base_get_xdg_surface(wm_base, surface);
    top_level = xdg_surface_get_toplevel(xdg_surf);
    xdg_toplevel_set_title(top_level, "Chip8 EMU");
    wayland::add_xdg_wm_base_listener(wm_base, *this);
    wayland::add_xdg_surface_listener(xdg_surf, *this);
}
void SurfaceManager::create_buffers() {
    buffers[0] = { wl_shm_pool_create_buffer(shm_pool, 0, display_width(), display_height(), display_width() * bpp,
                    WL_SHM_FORMAT_XRGB8888), true };
    buffers[1] = { wl_shm_pool_create_buffer(shm_pool, shm_pool_size / 2, display_width(), display_height(), display_width() * bpp,
                    WL_SHM_FORMAT_XRGB8888), true };
    for (auto [buf, is_valid] : buffers)
        wayland::add_buffer_listener(buf, *this);
};

void SurfaceManager::create_and_map_shm_pool(size_t pool_size) {
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
void SurfaceManager::xdg_surface_configure(xdg_surface* surf, uint32_t serial) {
    xdg_surface_ack_configure(surf, serial);
    wl_surface_attach(surface, buffers[0].first, 0, 0);
    wl_surface_commit(surface);
    buffers[0].second = false;
}
void SurfaceManager::buffer_release(wl_buffer* buffer) {
    auto it = ranges::find(buffers, buffer, &decltype(buffers)::value_type::first);
    it->second = true;
}
void SurfaceManager::callback_done(wl_callback* cb, uint32_t) {
    wl_callback_destroy(cb);
    cb = wl_surface_frame(surface);
    wayland::add_callback_listener(cb, *this);
    // chip8_mch.step();
    // write into the offset of the first free buffer
    const auto it = ranges::find(buffers, true, &decltype(buffers)::value_type::second);
    const auto off = ranges::distance(buffers.begin(), it);

    auto dst_pointer = reinterpret_cast<uint32_t*>(shm_pool_ptr.get<std::byte>() + (off * shm_pool_size / 2));
    blit_frame(
            state.chip8_mch.display.get_pixels(), 
            mdspan<uint32_t, dextents<uint32_t, 2>>(dst_pointer, display_height(), display_width()));
    wl_surface_attach(surface, it->first, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(surface);
}
void SurfaceManager::xdg_wm_base_ping(xdg_wm_base* base, uint32_t serial) {
    xdg_wm_base_pong(base, serial);
}
}
