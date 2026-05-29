#include <sys/mman.h>
#include <wayland-client-protocol.h>
#include "wayland-client.h"
#include "xdg-shell-client-protocol.h"
import std;
import chip8;
import wayland_wrapper;

using namespace std;
using namespace std::literals;
struct RegistryFunctions {
    function_ref<void(wl_registry*, uint32_t, string_view, uint32_t)> global;
    function_ref<void(wl_registry*, uint32_t)> global_remove;
};


int main() {
    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    xdg_wm_base* wm_base = nullptr;
    auto registry_listener = RegistryFunctions{
        .global = [&](wl_registry* reg, uint32_t name, string_view interface, uint32_t version) {
            std::ignore = reg;
            if (interface == wl_compositor_interface.name) {
                compositor = static_cast<wl_compositor*>(
                    wl_registry_bind(reg, name, &wl_compositor_interface, 4));
            } else if (interface == wl_shm_interface.name) {
                shm = static_cast<wl_shm*>(
                    wl_registry_bind(reg, name, &wl_shm_interface, 1));
            }
            println("interface: {}, version: {}, name: {}", interface, version, name);
        },
        .global_remove = [](wl_registry* reg, uint32_t name) {
            std::ignore = reg; std::ignore = name;
        }
    };
    auto display = wayland::Display::connect().value();
    // auto mch = chip8::Machine::from_file(filesystem::current_path() / "roms" / "ibm.ch8");
    // mch.execute();
    auto registry = display.get_registry();
    registry.add_listener(registry_listener);
    display.roundtrip();

    constexpr auto display_size_gcd = std::gcd(chip8::Display::width, chip8::Display::height);
    constexpr auto min_width = chip8::Display::width / display_size_gcd;
    constexpr auto min_height = chip8::Display::height / display_size_gcd;

    const auto scale = 300; // Probably read this from a file;
    const auto display_width = min_width * scale;
    const auto display_height = min_height * scale;
    const auto num_pixels = display_width * display_height;

    constexpr uint32_t bpp = 4, num_buffers = 2;
    const auto shm_pool_size = num_pixels * bpp * num_buffers;
    auto shm_file = wayland::create_shm_file()
        .and_then(bind_back<wayland::allocate_shm_file>(shm_pool_size))
        .value();
    
    wayland::MMappedPointer shm_pool_ptr{
        mmap(nullptr, shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_file.get(), 0),
        shm_pool_size
    };

    auto shm_pool = wl_shm_create_pool(shm, shm_file.get(), shm_pool_size);
    std::array<wl_buffer*, num_buffers> buffers = {
        wl_shm_pool_create_buffer(shm_pool, 0, display_width, display_height, display_width * bpp,
                                  WL_SHM_FORMAT_XRGB8888),
        wl_shm_pool_create_buffer(shm_pool, shm_pool_size / num_buffers, display_width, display_height, display_width * bpp,
                                  WL_SHM_FORMAT_XRGB8888)
    };
    
    return 0;
}
