export module chip8:executor;
import :machine;
import :display;
import std;

using namespace std;
export namespace chip8 {
void execute_instruction(const uint16_t inst, Machine& machine) {
    const auto first_nibble = (static_cast<uint32_t>(inst) >> 12u) & 0b1111;
    if (inst == 0x00E0) {
        machine.display.clear_screen();
        return;
    }
    // the last 3 nibbles
    const auto nnn = static_cast<uint16_t>(inst & 0x0FFF);
    // that last 2 nibbles
    const auto nn = static_cast<uint8_t>(inst & 0x00FF);
    // second nibble, also known as x
    const auto second_nibble = (static_cast<uint32_t>(inst) >> 8u) & 0b1111;
    // third nibble, also known as y
    const auto third_nibble = (static_cast<uint32_t>(inst) >> 4u) & 0b1111;
    // last nibble, also know as n
    const auto fourth_nibble = (static_cast<uint32_t>(inst) & 0b1111);

    // the first nibble of each instruction and what they should do
    constexpr uint32_t jump_nibble = 0x1;
    constexpr uint32_t set_nibble = 0x6;
    constexpr uint32_t add_nibble = 0x7;
    constexpr uint32_t set_index_nibble = 0xA;
    constexpr uint32_t draw_nibble = 0xD;

    // clang-format off
        switch (first_nibble) {
        break;case jump_nibble: {
            machine.state.pc = nnn;
        }
        break;case set_nibble: {
            // 6xnn set register v{x} to nn
            machine.state.registers[second_nibble] = nn;
        }
        break;case add_nibble: {
            // 7xnn add nn to the value at v{x}
            machine.state.registers[second_nibble] += nn;
        }
        break;case set_index_nibble: {
            machine.state.index = nnn;
        }
        break;case draw_nibble: {
            const auto posx = machine.state.registers[second_nibble] % Display::width;
            const auto posy = machine.state.registers[third_nibble] % Display::height;
            auto pixel_data = machine.display.get_pixels();
            // the fourth_nibble is the number of rows (or sprite height), width is 8
            // TODO: handle setting the VF register
            for (auto i : views::iota(0u, fourth_nibble)) {
                const auto row = machine.state.ram[machine.state.index + i];
                for (auto j : views::iota(0u, 8u)) {
                    const auto new_val = static_cast<uint8_t>(row >> (7u - j)) & 0b1;
                    if (posx + j >= Display::width || posy + i >= Display::height)
                        machine.display.get_pixels();
                    if (new_val == 1) {
                        auto& pixel = pixel_data[posx + j, posy + i];
                        if (pixel)
                            pixel = 0;
                        else pixel = numeric_limits<uint8_t>::max();
                    }
                }
            }
        }
        }
    // clang-format on
}
}  // namespace chip8
