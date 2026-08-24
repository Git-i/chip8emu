import std;
import chip8;
import wayland_wrapper;
import app;
using namespace std;
using namespace app;
consteval span<const uint32_t> render_game(
        span<const uint8_t> rom_data,
        uint32_t width,
        uint32_t num_steps
) {
    auto mch = chip8::Machine::from_rom_bytes(rom_data, +[](uint8_t, void*) { return false; }, nullptr)
        .value();
    for (auto _ : views::iota(0u, num_steps)) mch.step();
    if (width % chip8::Display::width != 0) {
        throw invalid_argument{"Width must be a multiple of chip8 display width"};
    }
    const auto height = width / (chip8::Display::width / chip8::Display::height);
    // std::vector<uint32_t> target_screen;
    // target_screen.resize(width * height);
    auto target_ptr = new uint32_t[width * height];
    SurfaceManager::blit_frame(
        mch.display.get_pixels(),
        mdspan<uint32_t, dextents<uint32_t, 2>>(target_ptr, height, width),
        0xFFB000, 0x382000
    );
    auto ret = define_static_array(span{target_ptr, width * height});
    delete[] target_ptr;
    return ret;
}
int main() { 
    constexpr uint8_t rom_data[] {
        #embed "../roms/ibm.ch8"
    };
    constexpr uint32_t output_width = 128;
    constexpr uint32_t output_height = output_width / 2;
    constexpr auto game_data = render_game(span{rom_data}, output_width, 70);
    std::cout << game_data.size() << std::endl;
    // Yes I'm aware its not optimal
    ofstream out_file("output_image.ppm", ios::binary);
    out_file << "P6\n" << output_width << " " << output_height << "\n255\n";
    ranges::copy(
        game_data
        | views::transform([](uint32_t input) {
            return std::array<uint8_t, 3>{
                static_cast<uint8_t>((input >> 16) & 0xFF),
                static_cast<uint8_t>((input >> 8)  & 0xFF),
                static_cast<uint8_t>(input & 0xFF)
            };
        })
        | views::join,
        ostreambuf_iterator<char>(out_file)
    );
    
}
