# Ports

A port is a platform layer that builds the game's own sources (`src/tu/`, unchanged from the matching build) for another machine. It stands in for the parts of the PS2 the game talks to: the EE's inline asm and VU0 macro ops, VU1 microcode, the GS, DMA and VIF, the IPU, the IOP sound driver, the pads, the memory card and the CD/DVD drive.

| Target | Directory | Status |
|---|---|---|
| Linux x86-64 | [`pc/`](pc/) | ✅ Whole game at 30 FPS |
| macOS, Apple silicon (native arm64) | [`pc/`](pc/) | ✅ Menus, races and movies at 30 FPS |
| macOS (x86-64; Apple silicon through Rosetta) | [`pc/`](pc/) | ✅ Menus and races at 30 FPS; movies need an x86-64 ffmpeg |
| Windows x86-64 (MinGW-w64) | [`pc/`](pc/) | 🔧 Builds and links; not yet run on Windows |

## Layout

```
ports/
  pc/        SDL3 desktop port (Linux, macOS, Windows; x86-64)
  <name>/    a new platform: add a directory, a README with its status, and a row above
tools/port/  build and test tooling for pc/ (rewrite, build, VU packets, golden images)
```

A new CPU on an existing OS goes in the existing port as a build target, not a new directory, so it shares the GS, VU1, audio and input code. A new directory is for a different platform API or hardware model.

## What every port shares

- **The game sources.** `tools/port/rewrite.py` turns the sources' inline asm into calls the port interprets (`pc/include/port_asm.h`, `pc/ee/asm_interp.cpp`). `pc/include/port_prelude.h` lets the C++98 game code build with a modern compiler. The matching build is never changed for a port, and `make` must keep its SHA-1.
- **32-bit pointers.** The game stores pointers in 32 bits (DMA tags, its file formats; `pc/include/port_ptr32.h`). A 64-bit port must keep the game's heap and globals below 4 GB.
- **The VU microcode packets,** which `tools/port/vu.py` takes from `make link`'s output, or, when that isn't there, straight from the game's executable in `baserom/` (its symbol table names them).
- **The checks.** `tools/port/golden.py` compares deterministic screenshots pixel for pixel. A port should pass it before it claims a mode works.
