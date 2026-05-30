export module chip8:executor;
import :machine;
import :display;
import std;

using namespace std;
export namespace chip8 {
enum class InstructionGroup : uint32_t {
    zero_nibble = 0x0,
    jump_nibble = 0x1,
    call_nibble = 0x2,
    skip_if_eq = 0x3,
    skip_if_neq = 0x4,
    skip_if_xy_eq = 0x5,
    set_nibble = 0x6,
    add_nibble = 0x7,
    alu_nibble = 0x8,
    skip_if_xy_neq = 0x9,
    set_index_nibble = 0xA,
    jump_with_offset_nibble = 0xB,
    random_nibble = 0xC,
    draw_nibble = 0xD,
    // this seems to do many things
    f_nibble = 0xF
};
constexpr InstructionGroup to_instruction(const uint32_t input) {
    return static_cast<InstructionGroup>(input);
}
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
    // println("{:#X}", inst);
    // the first nibble of each instruction and what they should do
    // clang-format off
        switch (to_instruction(first_nibble)) {
        using enum InstructionGroup;
        break;case jump_nibble: {
            machine.state.pc = nnn;
        }
        break;case call_nibble: {
            machine.state.stack.push_back(machine.state.pc);
            machine.state.pc = nnn;
        }
        break;case zero_nibble: {
            if (inst == 0x00EE) {
                auto back = machine.state.stack.back();
                machine.state.pc = back;
                machine.state.stack.pop_back();
            }
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
            machine.state.registers[0xF] = 0;
            for (auto i : views::iota(0u, fourth_nibble)) {
                const auto row = machine.state.ram[machine.state.index + i];
                for (auto j : views::iota(0u, 8u)) {
                    const auto new_val = static_cast<uint8_t>(row >> (7u - j)) & 0b1;
                    if (posx + j >= Display::width || posy + i >= Display::height) continue;
                    if (new_val == 1) {
                        auto& pixel = pixel_data[posy + i, posx + j];
                        if (pixel) {
                            pixel = 0;
                            machine.state.registers[0xF] = 1;
                        }
                        else pixel = numeric_limits<uint8_t>::max();
                    }
                }
            }
        }
        break;case skip_if_eq: {
            // 3XNN
            const auto val = machine.state.registers[second_nibble];
            if (val == nn) {
                machine.state.pc += 2;
            }
        }
        break;case skip_if_neq: {
            // 4XNN
            const auto val = machine.state.registers[second_nibble];
            if (val != nn) {
                machine.state.pc += 2;
            }
        }
        break;case skip_if_xy_eq: {
            // 5XY0
            if (
                machine.state.registers[second_nibble] ==
                machine.state.registers[third_nibble]
            ) {
                machine.state.pc += 2;
            }
        }
        break;case skip_if_xy_neq: {
            // 9XY0
            if (
                machine.state.registers[second_nibble] !=
                machine.state.registers[third_nibble]
            ) {
                machine.state.pc += 2;
            }
        }
        break;case alu_nibble: {
            // 8XY{something}
            auto& vx = machine.state.registers[second_nibble];
            auto& vy = machine.state.registers[third_nibble];
            auto do_sub = [&vx, &machine](uint8_t first, uint8_t second) {
                machine.state.registers[0xF] = first > second;
                vx = first - second;
            };
            if (fourth_nibble == 0) vx = vy;
            else if (fourth_nibble == 1) vx = vx | vy;
            else if (fourth_nibble == 2) vx = vx & vy;
            else if (fourth_nibble == 3) vx = vx ^ vy;
            else if (fourth_nibble == 4) {
                const auto result = vx + vy;
                vx = static_cast<uint8_t>(result);
                // set overflow
                machine.state.registers[0xF] = std::cmp_greater(result, numeric_limits<uint8_t>::max());
            }
            else if (fourth_nibble == 5) do_sub(vx, vy);
            else if (fourth_nibble == 7) do_sub(vy, vx);
            else if (fourth_nibble == 6) {
                // TODO: consider adding config to support setting vx to vy
                machine.state.registers[0xF] = vx & 0b1;
                vx = vx >> 1;
            }
            else if (fourth_nibble == 0xE) {
                machine.state.registers[0xF] = (vx >> 7) & 0b1;
                vx = vx << 1;
            } else println("Unimplemented 8 instruction: {:#X}", inst);
        }
        break;case jump_with_offset_nibble: {
            // BNNN
            machine.state.pc = nnn + machine.state.registers[0x0];
        }
        break;case random_nibble: {
            machine.state.registers[second_nibble] 
                = nn & machine.state.random_dist(machine.state.random_engine);
        }
        break;case f_nibble: {
            auto& vx = machine.state.registers[second_nibble];
            if (nn == 0x07) vx = machine.state.delay_timer;
            else if (nn == 0x33) {
                auto vx_copy = vx;
                machine.state.ram[machine.state.index + 2] = vx_copy % 10;
                vx_copy /= 10; machine.state.ram[machine.state.index + 1] = vx_copy % 10;
                vx_copy /= 10; machine.state.ram[machine.state.index] = vx_copy % 10;
            } else if (nn == 0x29) {
                uint8_t char_hex = vx & 0xF;
                // each char is 5 bytes
                machine.state.index = Machine::font_offset + (char_hex * 5);
            } else if (nn == 0x15) {
                println("15");
            } else if (nn == 0x55) {
                const auto register_range = span{machine.state.registers.data(), second_nibble + 1};
                ranges::copy(register_range, machine.state.ram.begin() + machine.state.index);
            } else if (nn == 0x65) {
                const auto memory_range = span{machine.state.ram.data() + machine.state.index, second_nibble + 1};
                ranges::copy(memory_range, machine.state.registers.data());
            } else if (nn == 0x1E) {
                machine.state.index += vx;
            } else println("f instruction {:#X}", inst);
        }
        break;default: {
            println("Not implemented {}", first_nibble);
        }
        }
    // clang-format on
}
}  // namespace chip8
