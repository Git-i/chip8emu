module;
#include <functional>
export module chip8:machine;
import :state;
import :display;
import std;
using namespace std;
export namespace chip8 {
struct Machine;
void execute_instruction(const uint16_t inst, Machine& machine);
struct Machine {
    State state{};
    Display display{};
    move_only_function<bool(uint8_t)> is_key_down;
    static constexpr size_t font_offset = 0x50;
    static auto from_file(
        const filesystem::path& path,
        move_only_function<bool(uint8_t)> is_key_down
    ) -> Machine {
        Machine out{.is_key_down = std::move(is_key_down)};
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
    void step() {
        const auto inst = 
            (static_cast<std::uint16_t>(state.ram[state.pc]) << 8) |
            state.ram[state.pc + 1];
        state.pc += 2;
        execute_instruction(inst, *this);
    }
    void advance_delay() {
        if (state.delay_timer != 0)
            state.delay_timer--;
    }
    void execute() {
        size_t i = 0;
        while (i < 2000) {
            i++;
            const auto inst =
                (static_cast<std::uint16_t>(state.ram[state.pc]) << 8) |
                state.ram[state.pc + 1];
            state.pc += 2;
            execute_instruction(inst, *this);
        }
        ofstream file{"out.ppm", ios::binary};
        file << std::format("P5\n{} {}\n255\n", Display::width,
                            Display::height);
        const auto raw_pixels = display.raw();
        file.write(reinterpret_cast<const char*>(raw_pixels.data()),
                   raw_pixels.size());
    }
};
}  // namespace chip8
