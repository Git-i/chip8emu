export module chip8:machine;
import :state;
import :display;
import std;
using namespace std;
export namespace chip8 {
struct Machine;
void execute_instruction(const uint16_t inst, Machine& machine);
struct Machine {
    State state;
    Display display;

    static Machine from_file(const filesystem::path& path) {
        Machine out;
        ifstream file{path};
        auto target_buffer = span{
            reinterpret_cast<char*>(out.state.ram.data()) + 512, 4_kb - 512};
        ospanstream stream{target_buffer};
        file >> stream.rdbuf();
        out.state.pc = 512;
        out.state.index = 0;
        return out;
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
