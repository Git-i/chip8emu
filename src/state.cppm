export module chip8:state;
import std;
export namespace chip8 {
constexpr std::size_t operator""_kb(unsigned long long in) { return in * 1024; }
struct State {
    std::array<std::uint8_t, 4_kb> ram;
    std::array<std::uint8_t, 16> registers{};
    std::uint16_t index;
    std::uint16_t pc;
};
}  // namespace chip8
