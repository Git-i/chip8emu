import std;
import chip8;

using namespace std;
int main(int argc, char **argv) {
    auto mch = chip8::Machine::from_file(filesystem::current_path() / "roms" / "ibm.ch8");
    mch.execute();
    return 0;
}
