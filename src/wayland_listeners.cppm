module;
#include <wayland-client-protocol.h>
#include "wayland-client.h"
#include "xdg-shell-client-protocol.h"
export module wayland_wrapper:listeners;
import std;

void wrap_xdg_wm_base_add_listener(xdg_wm_base* base, const xdg_wm_base_listener* lst, void* data) {
    xdg_wm_base_add_listener(base, lst, data);
}
void wrap_xdg_surface_add_listener(xdg_surface* srf, const xdg_surface_listener* lst, void* data) {
    xdg_surface_add_listener(srf, lst, data);
}
template<typename T>
constexpr static xdg_wm_base_listener xdg_wm_base_listener_for = xdg_wm_base_listener {
    .ping = +[](void* data, xdg_wm_base* base, uint32_t serial) {
        static_cast<T*>(data)->xdg_wm_base_ping(base, serial);
    }
};
template<typename T>
constexpr static xdg_surface_listener xdg_surface_listener_for = xdg_surface_listener {
    .configure = +[](void* data, xdg_surface* surf, uint32_t serial) {
        static_cast<T*>(data)->xdg_surface_configure(surf, serial);
    }
};
template<typename T>
constexpr static wl_buffer_listener buffer_listener_for = wl_buffer_listener {
    .release = +[](void* data, wl_buffer* buffer) {
        static_cast<T*>(data)->buffer_release(buffer);
    }
};
template<typename T>
constexpr static wl_callback_listener callback_listener_for = wl_callback_listener {
    .done = +[](void* data, wl_callback* cb, uint32_t time) {
        static_cast<T*>(data)->callback_done(cb, time);
    }
};
template<typename T>
constexpr static wl_seat_listener seat_listener_for = wl_seat_listener {
    .capabilities = +[](void* data, wl_seat* seat, uint32_t caps) {
        static_cast<T*>(data)->seat_capabilities(seat, caps);
    },
    .name = +[](void* data, wl_seat* seat, const char* name) {
        static_cast<T*>(data)->seat_name(seat, std::string_view{name});
    }
};
template<typename T>
constexpr static wl_keyboard_listener kbd_listener_for = wl_keyboard_listener {
};

export namespace wayland {
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
template<typename T>
concept SeatListener = requires(T a) {
    a.seat_capabilities((wl_seat*){nullptr}, uint32_t{0});
    a.seat_name((wl_seat*){nullptr}, std::string_view{});
};


template<XdgWmBaseListener T>
void add_xdg_wm_base_listener(xdg_wm_base* base, T& lst) {
    wrap_xdg_wm_base_add_listener(base, &xdg_wm_base_listener_for<T>, &lst);
}
template<XdgSurfaceListener T>
void add_xdg_surface_listener(xdg_surface* surf, T& lst) {
    wrap_xdg_surface_add_listener(surf, &xdg_surface_listener_for<T>, &lst);
}
template<BufferListener T>
void add_buffer_listener(wl_buffer* buf, T& lst) {
    wl_buffer_add_listener(buf, &buffer_listener_for<T>, &lst);
}
template<CallBackListener T>
void add_callback_listener(wl_callback* cb, T& lst) {
    wl_callback_add_listener(cb, &callback_listener_for<T>, &lst);
}
template<SeatListener T>
void add_seat_listener(wl_seat* st, T& lst) {
    wl_seat_add_listener(st, &seat_listener_for<T>, &lst);
}
}
