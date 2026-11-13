# PC port (SDL3)

The game's own code, unchanged from the matching build, compiled with clang and running on a PC version of the hardware it was written for. That hardware layer is the retail VU1 microcode in a recompiler that models the VU pipeline, a multithreaded software GS (AVX2) presented through SDL3, the IPU, PSS movies through libavcodec, a PC version of the IOP sound driver mixing to SDL3 audio, gamepads and the keyboard as DualShock 2 pads, and the memory card as a directory.

Targets. The game keeps pointers in 32 bits, so on x86-64 every address it sees lies below 4 GB; Apple silicon forbids that, so the native macOS build keeps the game's memory in a 4 GB window instead (`include/port_ptr32.h`). The software GS and VU1 are written with x86 vector intrinsics, which the arm64 build takes from SIMDe:

| Target | Output | Status |
|---|---|---|
| Linux | `build/port/splashdown` | ✅ whole game at 30 FPS |
| macOS 14.4+, Apple silicon, native (`--arch arm64`) | `build/port-macos-arm64/splashdown` | ✅ menus, races and movies at 30 FPS; about twice the Rosetta build's speed |
| macOS 14.4+ (Intel, or Apple silicon through Rosetta) | `build/port-macos/splashdown` | ✅ menus and races at 30 FPS |
| Windows 10+ (MinGW-w64) | `build/port-windows/splashdown.exe` | 🔧 builds and links, not yet run on Windows |

The disc must be the NTSC-U release (SLUS-20223): the game finds its files through a table of sectors compiled into it (`src/tu/MapFile.cpp`), so another region's disc fails at the first file.

## Build and run

Every target needs the game's executable for the VU microcode: either `make link`'s output (Linux, with the matching toolchain), or just the file copied from your disc:

```sh
mkdir -p baserom && bsdtar -xOf /path/to/splashdown.iso SLUS_202.23 > baserom/SLUS_202.23
```

### Linux

```sh
sudo pacman -S clang sdl3 ffmpeg zlib   # ffmpeg is optional: without it movies are skipped
python3 tools/port/build.py -j 8        # builds build/port/splashdown (--asan for an ASan build)
SPLASHDOWN_DISC=/path/to/splashdown.iso build/port/splashdown
```

### macOS

Xcode's command line tools build it. The libraries must be x86-64 (or universal): Homebrew's on Apple silicon are arm64 only, so SDL3 goes in a prefix of its own, `build/deps/macos/` (`include/SDL3/`, `lib/libSDL3.dylib`), or any directories named in `PORT_DEPS`. One way:

```sh
git clone --depth 1 -b release-3.4.20 https://github.com/libsdl-org/SDL
cmake -S SDL -B SDL/build -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=14.4 -DCMAKE_INSTALL_PREFIX=$PWD/build/deps/macos
cmake --build SDL/build -j 8 && cmake --install SDL/build
python3 tools/port/build.py -j 8
SPLASHDOWN_DISC=/path/to/splashdown.iso build/port-macos/splashdown
```

The build also assembles `build/port-macos/Splashdown.app`, which can be moved anywhere and opened from Finder. Its launcher (`ports/pc/macos/launch`) uses the first `.iso` or `.bin` in the folder the app is in, else asks for one and remembers it, and keeps the game's output in `~/Library/Logs/Splashdown.log`.

An x86-64 ffmpeg (libavcodec, libavutil) installed in the same prefix turns movie playback on. zlib is the system's. Saves go to `~/Library/Application Support/splashdown/memcard0/`.

#### Native Apple silicon

```sh
brew install sdl3 ffmpeg pkgconf
mkdir -p build/deps/simde/simde/x86     # SIMDe's amalgamated x86/avx2.h (github.com/simd-everywhere/simde releases)
cp /path/to/simde-amalgamated/x86/avx2.h build/deps/simde/simde/x86/
python3 tools/port/build.py --arch arm64 -j 8
SPLASHDOWN_DISC=/path/to/splashdown.iso build/port-macos-arm64/splashdown
```

Homebrew's libraries are arm64, so this build uses them directly (and plays the movies). How it differs from the x86-64 builds:

- **The window.** A 32-bit value `v` stands for the address `0x100000000 + v`, and 0 for null: `Ptr32` (`PS2_PTR`), `PS2_ADDR(x)` at the 39 places the game casts an integer back to a pointer, and `PORT_HW(addr)` for register addresses (`tools/port/rewrite.py` adds it to the game's literals). Truncating a pointer gives its 32-bit value back. On x86-64 all three are the identity.
- **Where things are.** The executable loads at the window's start once address randomization is off (`os.cpp` re-executes for that). The EE and GS registers and the scratchpad are segments of the executable at their addresses in the window (`-segaddr`; `hw.cpp`), and the game's heap is an array in it (`lowheap.cpp`): by the time the port's code runs, the system has taken the addresses after the executable.
- **Arithmetic.** Built with `-ffp-contract=off`, so floating point rounds as on x86-64. The golden session's race frames match the x86-64 build's pixel for pixel; the animated water behind the menus does not (cause not found).

A missed conversion shows as a fault at an address below 4 GB: that address is the game's 32-bit value.

### Windows

Cross-built from Linux or macOS with clang, lld and MinGW-w64 (`brew install llvm lld mingw-w64`; set `CXX` to that clang++), or built in an MSYS2 CLANG64 shell. The prefix is `build/deps/windows/`: SDL3's MinGW development package (`include/SDL3/`, `lib/libSDL3.dll.a`, `bin/SDL3.dll`), a static zlib (`include/zlib.h`, `lib/libz.a`), and optionally ffmpeg's shared build (`include/libavcodec/`, `lib/libavcodec.dll.a`, `bin/avcodec-*.dll` ...).

```sh
CXX=/opt/homebrew/opt/llvm/bin/clang++ python3 tools/port/build.py --target windows -j 8
```

The build copies the DLLs it finds in the prefix next to `splashdown.exe`, with `Play Splashdown.bat` (it starts the game and keeps its output in `splashdown-log.txt`) and a `README.txt` for whoever gets the folder (`ports/pc/windows/`). Saves go to `%APPDATA%\splashdown\memcard0\`.

### All targets

The disc image is `$SPLASHDOWN_DISC`, else the command line's first argument (an image dropped on the executable), else the first `.iso` or `.bin` beside the executable, else `baserom/splashdown.bin`. `PORT_MEMCARD_DIR` moves the saves. On Linux a window opens when `WAYLAND_DISPLAY` or `DISPLAY` is set; macOS and Windows always open one. `PORT_HEADLESS=1` runs without, which the test tools use (`PORT_SCREENSHOT`, `PORT_INPUT`, `tools/port/golden.py`).

What differs off Linux (`os.cpp`, `lowheap.cpp`, `nodebug.cpp`): the game's heap is the port's own allocator below 256 MB instead of glibc's brk heap; the executable is linked at 0x400000 without address randomization (macOS: a 4 KB `__PAGEZERO`, which only x86-64 allows, hence Rosetta); the hang watchdog, the sampling profiler and `PORT_HEAPGUARD` are Linux only.

## Display settings and widescreen

**F1** (or the gamepad's guide button, or both sticks pressed together) opens the port's settings over the game: Up/Down pick a row, Left/Right change it, Esc closes; the mouse works too. The game keeps running underneath and sees no input meanwhile. The settings are kept in `settings.ini` in the data directory (beside `memcard0/`).

| Setting | Values |
|---|---|
| Aspect ratio | 4:3 Original, 16:9 Widescreen, 16:10, 21:9 Ultrawide, Fill window |
| Display | Windowed, Fullscreen (borderless, the desktop's resolution) |
| Window size | 1x to 4x the game's 448 lines |
| Scaling | Smooth (linear), Sharp (nearest), Integer (whole multiples) |
| VSync | Off, On |
| FPS counter | Off, On |

Widescreen is a wider field of view, not a stretched picture: the game still renders 640x448 (the GS is the PS2's, in software), `ports/pc/game/widescreen_pc.cpp` widens the projection's horizontal field of view by the shape's ratio to 4:3, and the presenter shows the picture in that shape. Clipping and culling follow the projection; the near-water patch is widened to match (`PSXWaterDisplacement.cpp`, `SPLASHDOWN_PC`). What doesn't go through a projection is stretched with the picture: the HUD, the menus and the movies. There is no higher internal resolution or anti-aliasing: that would take a different rasterizer.

For tests: `PORT_ASPECT=16:9` sets the field of view without a window (headless runs ignore `settings.ini`, so the golden session is unchanged); `PORT_OVERLAY_OPEN=1` starts with the menu open and `PORT_WINDOW_SHOT=out.png` saves what the window shows.

## Controls

Any SDL3 gamepad (Xbox, PlayStation, ...) in the standard layout works: the first one is player 1 and a second one is player 2 for Vs. Analog triggers and rumble work. Xbox A/B/X/Y are Cross/Circle/Square/Triangle, Menu is Start and View is Select.

Keyboard: arrows (D-pad), WASD (left stick), IJKL (right stick), X or Space (Cross), C or Esc (Circle), Z (Square), V (Triangle), Enter (Start), Tab (Select), Q/E (L1/R1), 1/3 (L2/R2).

## Layout

```
main.cpp, runtime.cpp   start-up, the game's thread, the port heap
os.cpp                  the host OS: futexes, low memory, threads, paths
settings.cpp, overlay.cpp  the port's display settings and their F1 menu
lowheap.cpp, nodebug.cpp  macOS and Windows: the game's heap; stand-ins
                        for the Linux-only watchdog and profiler
ee/                     the EE inline-asm / VU0 macro interpreter
vu/                     VIF1, VU1 interpreter and recompiler
gs/                     software GS rasterizer
dma.cpp, hw.cpp         DMA channels and hardware registers
ipu.cpp, video.cpp      IPU decoding, presentation through SDL3/OpenGL
input.cpp               SDL3 gamepads and the keyboard
audio/                  the IOP sound driver, mixed to SDL3 audio
sdk/                    Sony SDK replacements: CD/DVD, pads, memory card, MPEG, sound RPC
game/                   game-side code the PS2 build has as asm (C versions)
include/                prelude, 32-bit pointers, the asm interface
```
