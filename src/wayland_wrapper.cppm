module;
#include <wayland-client-protocol.h>
#include "wayland-client.h"
#include "fcntl.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "unistd.h"
#include "errno.h"
#include "xdg-shell-client-protocol.h"
export module wayland_wrapper;
import std;

template<typename T>
struct registry_listener_for {
    constexpr static wl_registry_listener data = wl_registry_listener {
        .global = +[](void* data, wl_registry* reg, uint32_t name, const char* itf, uint32_t version) {
            static_cast<T*>(data)->registry_global(reg, name, std::string_view{itf}, version);
        },
        .global_remove = +[](void* data, wl_registry* reg, uint32_t name) {
            static_cast<T*>(data)->registry_global_remove(reg, name);
        }
    };
};
template<typename T>
struct xdg_wm_base_listener_for {
    constexpr static xdg_wm_base_listener data = xdg_wm_base_listener {
        .ping = +[](void* data, xdg_wm_base* base, uint32_t serial) {
            static_cast<T*>(data)->xdg_wm_base_ping(base, serial);
        }
    };
};
template<typename T>
struct xdg_surface_listener_for {
    constexpr static xdg_surface_listener data = xdg_surface_listener {
        .configure = +[](void* data, xdg_surface* surf, uint32_t serial) {
            static_cast<T*>(data)->xdg_surface_configure(surf, serial);
        }
    };
};
template<typename T>
struct buffer_listener_for {
    constexpr static wl_buffer_listener data = wl_buffer_listener {
        .release = +[](void* data, wl_buffer* buffer) {
            static_cast<T*>(data)->buffer_release(buffer);
        }
    };
};
template<typename T>
struct callback_listener_for {
    constexpr static wl_callback_listener data = wl_callback_listener {
        .done = +[](void* data, wl_callback* cb, uint32_t time) {
            static_cast<T*>(data)->callback_done(cb, time);
        }
    };
};



void wrap_xdg_wm_base_add_listener(xdg_wm_base* base, const xdg_wm_base_listener* lst, void* data) {
    xdg_wm_base_add_listener(base, lst, data);
}
void wrap_xdg_surface_add_listener(xdg_surface* srf, const xdg_surface_listener* lst, void* data) {
    xdg_surface_add_listener(srf, lst, data);
}
export namespace wayland {
struct DisplayDeleter {
    static void operator()(wl_display* in) {
        wl_display_disconnect(in);
    }
};
using DisplayPtr = std::unique_ptr<wl_display, DisplayDeleter>;
template<typename T>
concept RegistryListener = requires (T a)  {
    a.registry_global((wl_registry*){nullptr}, uint32_t{0}, std::string_view{}, uint32_t{0});
    a.registry_global_remove((wl_registry*){nullptr}, uint32_t{0});
};
template<typename T>
concept XdgWmBaseListener = requires (T a) {
    a.xdg_wm_base_ping((xdg_wm_base*){nullptr}, uint32_t{0});
};
template<typename T>
concept XdgSurfaceListener = requires (T a) {
    a.xdg_surface_configure((xdg_surface*){nullptr}, uint32_t{0});
};
template<typename T>
concept BufferListener = requires (T a) {
    a.buffer_release((wl_buffer*){nullptr});
};
template<typename T>
concept CallBackListener = requires (T a) { 
    a.callback_done((wl_callback*){nullptr}, uint32_t{0});
};


template<XdgWmBaseListener T>
void add_xdg_wm_base_listener(xdg_wm_base* base, T& lst) {
    wrap_xdg_wm_base_add_listener(base, &xdg_wm_base_listener_for<T>::data, &lst);
}
template<XdgSurfaceListener T>
void add_xdg_surface_listener(xdg_surface* surf, T& lst) {
    wrap_xdg_surface_add_listener(surf, &xdg_surface_listener_for<T>::data, &lst);
}
template<BufferListener T>
void add_buffer_listener(wl_buffer* buf, T& lst) {
    wl_buffer_add_listener(buf, &buffer_listener_for<T>::data, &lst);
}
template<CallBackListener T>
void add_callback_listener(wl_callback* cb, T& lst) {
    wl_callback_add_listener(cb, &callback_listener_for<T>::data, &lst);
}

class Registry {
    wl_registry* m_ptr;
public:
    Registry(wl_registry* ptr) : m_ptr(ptr) {};
    [[nodiscard]] auto get_ptr() noexcept -> wl_registry* {
        return m_ptr;
    }
    template<RegistryListener T>
    void add_listener(T& listener) noexcept {
        wl_registry_add_listener(m_ptr, &registry_listener_for<T>::data, &listener);
    }
};
class Display {
    DisplayPtr m_ptr;
public:
    Display(wl_display* dp) : m_ptr(dp) {}
    [[nodiscard]] static auto connect() noexcept -> std::expected<Display, std::monostate> {
        auto display = wl_display_connect(nullptr);
        if (!display) {
            return std::unexpected(std::monostate{});
        }
        return std::expected<Display, std::monostate>{std::in_place, display};
    }
    [[nodiscard]] auto dispatch() noexcept -> int {
        return wl_display_dispatch(m_ptr.get());
    }
    [[nodiscard]] auto get_registry() noexcept -> Registry {
        return Registry{wl_display_get_registry(m_ptr.get())};
    }
    void roundtrip() noexcept {
        wl_display_roundtrip(m_ptr.get());
    }
};
class MMappedPointer {
    void* m_ptr; size_t m_size;
public:
    MMappedPointer(void* ptr, size_t size)
        : m_ptr(ptr), m_size(size) {}

    template<typename T>
    [[nodiscard]] auto get() noexcept -> T* {
        return static_cast<T*>(m_ptr);
    }
    MMappedPointer(const MMappedPointer&) = delete;
    MMappedPointer(MMappedPointer&& other) {
        m_ptr = other.m_ptr; m_size = other.m_size;
        other.m_ptr = nullptr;
    }
    MMappedPointer& operator=(const MMappedPointer&) = delete;
    MMappedPointer& operator=(MMappedPointer&& other) {
        if (m_ptr) munmap(m_ptr, m_size);
        m_ptr = nullptr; std::swap(other.m_ptr, m_ptr);
        std::swap(m_size, other.m_size);
        return *this;
    }
    ~MMappedPointer() { if(m_ptr) munmap(m_ptr, m_size); }
};
class UniqueHandle {
    int m_ID = -1;
public:
    UniqueHandle(int id) : m_ID(id) {}
    ~UniqueHandle() {
        if (m_ID != -1) close(m_ID);
    }
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle(UniqueHandle&& other) {
        m_ID = other.m_ID; other.m_ID = -1;
    }
    UniqueHandle& operator=(const UniqueHandle&) = delete;
    UniqueHandle& operator=(UniqueHandle&& other) {
        if (m_ID != -1) close(m_ID);
        m_ID = -1; std::swap(m_ID, other.m_ID);
        return *this;
    }
    [[nodiscard]] auto get() noexcept -> int {
        return m_ID;
    }
};
std::optional<UniqueHandle> create_shm_file() {
    using namespace std::string_view_literals;

    constexpr auto num_retries = 100u;
    char name[] = "/wl-shm-XXXXXX";
    std::span<char, 6> end_portion{ name + "/wl-shm-"sv.size(), 6 };
    std::philox4x32 eng; // can optionally seed this with random device, but nah
    std::uniform_int_distribution<uint32_t> dist(0u, 'Z' - 'A');
    for (const auto _ : std::views::iota(0u, num_retries)) {
        std::ranges::generate(end_portion, [&] { 
            return static_cast<char>('A' + dist(eng)); 
        });
        int fd = shm_open(name, O_RDWR | O_EXCL | O_CREAT, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return std::optional<UniqueHandle>{std::in_place, fd};
        }
    }
    return std::nullopt;
}
std::optional<UniqueHandle> allocate_shm_file(UniqueHandle&& handle, size_t size) {
    while (true) {
        const auto ret = ftruncate(handle.get(), size);
        if (ret == 0) return std::move(handle);
        if (errno != EINTR) break;
    }
    return std::nullopt;
}

}
