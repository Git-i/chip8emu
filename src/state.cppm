export module chip8:state;
import std;
using namespace std;
export namespace chip8 {
constexpr size_t operator""_kb(unsigned long long in) { return in * 1024; }
struct State {
    philox4x32 random_engine;
    uniform_int_distribution<uint32_t> random_dist
        {0, numeric_limits<uint8_t>::max()};
    array<uint8_t, 4_kb> ram;
    array<uint8_t, 16> registers{};
    uint16_t index;
    uint16_t pc;
};
}  // namespace chip8
