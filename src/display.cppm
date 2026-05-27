export module chip8:display;
import std;
using namespace std;
export namespace chip8 {
struct Display {
    static constexpr uint32_t height = 32;
    static constexpr uint32_t width = 64;
    // we could probably do with an std::bitset here, but I doubt its worth it
    // also this is row major
    using Extents = extents<uint32_t, height, width>;

private:
    array<uint8_t, width * height> pixels{};

public:
    void clear_screen() { ranges::fill(pixels, 0); }
    auto get_pixels() -> mdspan<uint8_t, Extents> {
        return mdspan(pixels.data(), Extents());
    }
    auto raw() -> span<uint8_t, width * height> { return pixels; }
};
}  // namespace chip8
