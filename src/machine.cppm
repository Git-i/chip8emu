export module chip8:machine;
import :state;
import :display;
import std;
using namespace std;
export namespace chip8 {
struct Machine {
    State state{};
    Display display{};
    move_only_function<bool(uint8_t)> is_key_down;
    static constexpr size_t font_offset = 0x50;
    static auto from_file(
        const filesystem::path& path,
        move_only_function<bool(uint8_t)> is_key_down
    ) -> Machine {
        Machine out{ .is_key_down = std::move(is_key_down)};
        out.state.stack.reserve(10); // tried to use inplace_vector but gcc(16.1) is bugged
        ifstream file{path};
        auto target_buffer = span{
            reinterpret_cast<char*>(out.state.ram.data()) + 512, 4_kb - 512};
        ospanstream stream{target_buffer};
        file >> stream.rdbuf();
        out.state.pc = 512;
        out.state.index = 0;

        // generate the font at 0x50
        auto font_data = array<uint8_t, 5 /* bytes per char */ * 16 /* num chars */> {
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
        ranges::copy(font_data, out.state.ram.data() + font_offset);
        return out;
    }
    void step();
    void advance_delay() {
        if (state.delay_timer != 0)
            state.delay_timer--;
    }
};
}  // namespace chip8
