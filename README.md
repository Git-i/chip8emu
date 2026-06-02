# Chip-8

A fairly simple chip-8 emulator, written in C++26 (whatever parts clang had of it),
using xmake and modules. It uses wayland for display and input so it won't work
on windows or mac (maybe it'll work on BSD).

## Building and running

It requires:

- xmake
- wayland-client
- wayland-protocols
- pkg-config

With all dependencies, it can be run via:

```sh
xmake run chip8 <path to rom>
```
