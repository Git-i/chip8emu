export module chip8:machine;
import :state;
import :display;
import std;
using namespace std;
export namespace chip8 {
struct Machine {
    State state{};
    Display display{};
    bool(*is_key_down)(uint8_t, void*);
    void* callback_data;
    static constexpr size_t font_offset = 0x50;
    static constexpr auto font_data = array<uint8_t, 5 /* bytes per char */ * 16 /* num chars */> {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    static constexpr auto from_rom_bytes(
        span<const uint8_t> data,
        bool (*is_key_down)(uint8_t, void*),
        void* callback_data
    ) -> optional<Machine> {
        Machine out{.is_key_down = is_key_down, .callback_data = callback_data };
        out.state.stack.reserve(10);
        if (data.size() > 4_kb - 512) return nullopt;
        ranges::copy(data, out.state.ram.data() + 512);
        out.state.pc = 512;
        out.state.index = 0;
        ranges::copy(font_data, out.state.ram.data() + font_offset);
        return out;
    }
    static auto from_file(
        const filesystem::path& path,
        bool (*is_key_down)(uint8_t, void*),
        void* callback_data
    ) -> Machine {
        Machine out{ .is_key_down = is_key_down, .callback_data = callback_data };
        out.state.stack.reserve(10); // tried to use inplace_vector but gcc(16.1) is bugged
        ifstream file{path};
        auto target_buffer = span{
            reinterpret_cast<char*>(out.state.ram.data()) + 512, 4_kb - 512};
        ospanstream stream{target_buffer};
        file >> stream.rdbuf();
        out.state.pc = 512;
        out.state.index = 0;
        // generate the font at 0x50
        ranges::copy(font_data, out.state.ram.data() + font_offset);
        return out;
    }
    constexpr void step();
    void advance_delay() {
        if (state.delay_timer != 0)
            state.delay_timer--;
    }
};
}  // namespace chip8
