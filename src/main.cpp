import std;
import chip8;
import wayland_wrapper;
import app;
using namespace std;

vector<string_view> read_cmd_args(int argc, char** argv) {
    vector<string_view> result;
    result.reserve(argc);
    for (const auto i : views::iota(0, argc))
        result.emplace_back(argv[i]);
    return result;
}
int main(int argc, char** argv) {
    auto args = read_cmd_args(argc, argv);
    if (args.size() != 2) {
        println("Usage: {} [path to rom]", args[0]);
        return 1;
    }
    const auto path = filesystem::path(args[1]);
    if (!filesystem::exists(path)) {
        println("The file {} does not exist", path);
        return 1;
    }
    app::State wl_state{path};
    while (wl_state.dispatch()) { 
        for(auto _ : views::iota(0u, 30u)) wl_state.chip8_mch.step(); 
    }
}
