#!/usr/bin/env python3
"""Build the PC port: rewrite the sources' inline asm
(tools/port/rewrite.py), compile every game TU with clang++
for x86-64, compile ports/pc/ and link the executable:

  Linux     build/port/splashdown
  macOS     build/port-macos/splashdown          (x86-64: runs natively on
            Intel Macs and under Rosetta on Apple silicon)
  Windows   build/port-windows/splashdown.exe    (MinGW-w64; cross-built
            from Linux or macOS, or built in an MSYS2 CLANG64/UCRT64 shell)

The target is the host's unless --target says otherwise. macOS and Windows
take SDL3 (and optionally ffmpeg, zlib) from a prefix: see ports/pc/README.md.

The MSL runtime (src/tu/msl/) and zlib 1.0.4 (src/tu/zlib/) are left out:
the host's C++ runtime and zlib (-lz) replace them.

usage: tools/port/build.py [-j N] [--target linux|macos|windows] [--keep-going] [--only NAME ...] [--no-link] [--asan]

--asan builds build/port-asan/splashdown with AddressSanitizer, to find
overflows (its heap is above 4 GB, so a truncated heap pointer crashes there).
"""
import argparse
import concurrent.futures
import glob
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "build/port")
TREE = os.path.join(ROOT, "build/port/tree")

CXX = os.environ.get("CXX", "clang++")
# The game code: C++98 as mwcc took it, warnings off (they are the
# matching build's business), the PC prelude first.
GAME_FLAGS = [
    "-std=gnu++98", "-m64", "-fno-pie", "-g", "-O1", "-w", "-fms-extensions", "-fms-compatibility",
    "-Wno-address-of-temporary", "-Wno-error=return-mismatch", "-fno-exceptions", "-fno-strict-aliasing", "-fwrapv", "-fno-delete-null-pointer-checks",
    # Retail functions that fall off the end without a return value (mwcc
    # leaves $v0 as is): without this clang takes those paths for unreachable
    # and drops the branch (PSXMediaControl::Stop ran its body when stopped).
    "-fno-strict-return",
    "-include", os.path.join(ROOT, "ports/pc/include/port_prelude.h"),
    "-I" + os.path.join(TREE, "src"), "-I" + os.path.join(TREE, "include"),
    "-I" + os.path.join(ROOT, "ports/pc/include"),
]
PORT_FLAGS = [
    "-std=c++20", "-m64", "-fno-pie", "-g", "-O2", "-Wall",
    "-I" + os.path.join(ROOT, "ports/pc/include"), "-I" + os.path.join(ROOT, "ports/pc"),
]
# The VU microcode upload packets (tools/port/vu.py gen), assembled.
VU_PACKETS = os.path.join(OUT, "gen/vupackets.s")
LIBS = ["-lSDL3", "-lGL", "-lz", "-lm"]
LINK_FLAGS = ["-m64", "-no-pie", "-g", "-pthread"]
ASM_FLAGS = ["-m64", "-fno-pie"]
EXE = "splashdown"
VU_FORMAT = "elf"
# macOS and Windows: the game's allocations go to ports/pc/lowheap.cpp.
LOWHEAP_DEFS = ["-Dmalloc=port_malloc", "-Dfree=port_free", "-Dcalloc=port_calloc",
                "-Drealloc=port_realloc", "-Dmemalign=port_memalign"]


# True when the target's libraries are the build host's own (pkg-config finds
# them): Linux, and macOS built for the host's CPU.
HOST_LIBS = True


def host_target():
    return {"darwin": "macos", "win32": "windows", "cygwin": "windows", "msys": "windows"}.get(sys.platform, "linux")


def prefix_flags(prefixes):
    """-I and -L for each dependency prefix (a directory with include/ and lib/)."""
    cflags, libs = [], []
    for p in prefixes:
        if os.path.isdir(os.path.join(p, "include")):
            cflags.append("-I" + os.path.join(p, "include"))
        if os.path.isdir(os.path.join(p, "lib")):
            libs.append("-L" + os.path.join(p, "lib"))
    return cflags, libs


def dep_prefixes(target):
    """$PORT_DEPS (a path list), then build/deps/<target>/ if it exists:
    where SDL3, and zlib and ffmpeg if wanted, were installed for the target."""
    out = [p for p in os.environ.get("PORT_DEPS", "").split(os.pathsep) if p]
    local = os.path.join(ROOT, "build/deps", target)
    if os.path.isdir(local):
        out.append(local)
    return out


def configure(target, arch="x86_64"):
    """Set the flags for a target other than Linux. The port needs every
    game address below 4 GB and the executable's own below 256 MB (ports/pc/os.cpp):
    both link at 0x400000, without address randomization."""
    global OUT, EXE, VU_FORMAT, CXX
    OUT = os.path.join(ROOT, "build/port-" + target + ("-arm64" if arch == "arm64" else ""))
    cflags, libdirs = prefix_flags(dep_prefixes(target))
    GAME_FLAGS.extend(LOWHEAP_DEFS)
    if target == "macos" and arch == "arm64":
        # Native Apple silicon: the game's memory is a 4 GB window at the
        # executable's default address (ports/pc/include/port_ptr32.h), so
        # no link options; the x86 vector code goes through SIMDe
        # (build/deps/simde/, or a PORT_DEPS prefix); SDL3, zlib and ffmpeg
        # are the host's (Homebrew's are arm64), found by pkg-config.
        VU_FORMAT = "macho"
        # -ffp-contract=off: arm64 compilers fuse a*b+c into one rounding
        # by default, x86-64 ones don't; without it the game's arithmetic
        # (and so its physics and the golden frames) drifts from x86-64's
        archf = ["-arch", "arm64", "-mmacosx-version-min=14.4", "-ffp-contract=off"]
        for flags in (GAME_FLAGS, PORT_FLAGS, ASM_FLAGS, LINK_FLAGS):
            flags[:] = [f for f in flags if f not in ("-m64", "-no-pie", "-fno-pie", "-pthread")] + archf
        simde = [p for p in dep_prefixes(target) + [os.path.join(ROOT, "build/deps/simde")]
                 if os.path.exists(os.path.join(p, "simde/x86/avx2.h"))]
        if not simde:
            sys.exit("build.py: the arm64 build needs SIMDe's x86/avx2.h as build/deps/simde/simde/x86/avx2.h "
                     "(ports/pc/README.md)")
        PORT_FLAGS.append("-I" + simde[0])
        pc = subprocess.run(["pkg-config", "--cflags", "--libs", "sdl3"], capture_output=True, text=True)
        if pc.returncode:
            sys.exit("build.py: pkg-config can't find sdl3 (brew install sdl3)")
        words = pc.stdout.split()
        PORT_FLAGS.extend(w for w in words if w.startswith("-I") or w.startswith("-D"))
        LIBS[:] = [w for w in words if w.startswith("-L") or w.startswith("-l")] + ["-lz"]
        # the PS2's register windows and scratchpad, as segments at their
        # addresses in the game's window (ports/pc/hw.cpp)
        LINK_FLAGS.extend(["-Wl,-segaddr,__PS2EE,0x110000000", "-Wl,-segaddr,__PS2GS,0x112000000",
                           "-Wl,-segaddr,__PS2SPR,0x170000000"])
        return [], []
    if target == "macos":
        VU_FORMAT = "macho"
        # os_sync_wait_on_address (the port's futex) is macOS 14.4's
        arch = ["-arch", "x86_64", "-mmacosx-version-min=14.4"]
        for flags in (GAME_FLAGS, PORT_FLAGS, ASM_FLAGS, LINK_FLAGS):
            flags[:] = [f for f in flags if f not in ("-m64", "-no-pie", "-pthread")] + arch
        LINK_FLAGS.extend(["-Wl,-pagezero_size,0x1000", "-Wl,-image_base,0x400000", "-Wl,-no_pie"])
        LIBS[:] = libdirs + ["-lSDL3", "-lz"]
        # SDL3's install name is @rpath/...: beside the executable (the
        # build copies it there), then where it was installed
        LINK_FLAGS.append("-Wl,-rpath,@loader_path")
        for d in libdirs:
            LINK_FLAGS.append("-Wl,-rpath," + d[2:])
    else:
        VU_FORMAT = "coff"
        EXE = "splashdown.exe"
        cross = []
        if host_target() != "windows":
            cross = ["--target=x86_64-w64-mingw32"]
            sysroot = os.environ.get("PORT_MINGW_SYSROOT")
            if not sysroot:
                gcc = shutil.which("x86_64-w64-mingw32-gcc")
                if gcc:
                    # <prefix>/bin/x86_64-w64-mingw32-gcc: the sysroot is <prefix>
                    sysroot = os.path.dirname(os.path.dirname(os.path.realpath(gcc)))
            if sysroot:
                cross.append("--sysroot=" + sysroot)
        # M_PI; gmtime_r; and no _MSC_VER from -fms-compatibility (MinGW's
        # headers would take clang for MSVC)
        win = cross + ["-D_FILE_OFFSET_BITS=64", "-D_WIN32_WINNT=0x0A00", "-D_USE_MATH_DEFINES",
                       "-D_POSIX_THREAD_SAFE_FUNCTIONS", "-fmsc-version=0", "-fgnuc-version=4.2.1"]
        for flags in (GAME_FLAGS, PORT_FLAGS, ASM_FLAGS):
            flags[:] = [f for f in flags if f != "-fno-pie"] + win
        # The game's own __do_global_dtors (retail's start-up code,
        # GameObject.cpp) has the name of MinGW's.
        GAME_FLAGS.append("-D__do_global_dtors=splashdown_do_global_dtors")
        LINK_FLAGS[:] = cross + ["-g", "-static", "-fuse-ld=lld", "-Wl,--image-base,0x400000",
                                 "-Wl,--disable-dynamicbase", "-Wl,--disable-high-entropy-va",
                                 "-Wl,--stack,0x800000"]
        # SDL3 as a DLL (its import library); the C++ runtime, winpthreads and zlib static
        LIBS[:] = libdirs + ["-Wl,-Bdynamic", "-lSDL3", "-Wl,-Bstatic", "-lz", "-lpthread", "-lsynchronization"]
    PORT_FLAGS.extend(cflags)
    return cflags, libdirs


def ffmpeg_flags(target, prefixes):
    """libavcodec/libavutil (movies, ports/pc/sdk/mpeg.cpp): (cflags, libs), or
    None if they aren't installed (PORT_NO_FFMPEG=1 forces that): the port
    is then built with movies stubbed (two black frames). Linux: from
    pkg-config. macOS, Windows: from a dependency prefix that has them (the
    host's pkg-config would name libraries of the wrong architecture)."""
    if os.environ.get("PORT_NO_FFMPEG", "0") != "0":
        return None
    if target != "linux" and HOST_LIBS is False:
        for p in prefixes:
            if os.path.exists(os.path.join(p, "include/libavcodec/avcodec.h")):
                return [], ["-lavcodec", "-lavutil"]
        return None
    try:
        p = subprocess.run(["pkg-config", "--cflags", "--libs", "libavcodec", "libavutil"],
                           capture_output=True, text=True)
    except FileNotFoundError:
        return None
    if p.returncode:
        return None
    cflags = subprocess.run(["pkg-config", "--cflags", "libavcodec", "libavutil"],
                            capture_output=True, text=True).stdout.split()
    libs = subprocess.run(["pkg-config", "--libs", "libavcodec", "libavutil"],
                          capture_output=True, text=True).stdout.split()
    return cflags, libs


def game_sources():
    srcs = sorted(glob.glob(os.path.join(ROOT, "src/tu/*.cpp")))
    return srcs


def port_sources():
    # ports/pc/game/ holds game-side code (C versions of asm, template
    # instantiations): it is built like the game TUs.
    srcs = sorted(glob.glob(os.path.join(ROOT, "ports/pc/**/*.cpp"), recursive=True))
    return [s for s in srcs if "/ports/pc/game/" not in s]


def port_game_sources():
    return sorted(glob.glob(os.path.join(ROOT, "ports/pc/game/*.cpp")))


def compile_one(src, flags, tree_src):
    rel = os.path.relpath(src, ROOT)
    obj = os.path.join(OUT, "obj", rel.replace("build/port/", "") + ".o")
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    inp = os.path.join(TREE, rel) if tree_src else src
    if rel == "src/tu/PSXMain.cpp":
        # The game's main runs on ports/pc/main.cpp's thread.
        flags = flags + ["-Dmain=splashdown_main"]
    if rel == "src/tu/Math3d.cpp":
        # Its ProjectionMatrix is wrapped by ports/pc/game/widescreen_pc.cpp.
        flags = flags + ["-DProjectionMatrix=ProjectionMatrix_retail"]
    deps = [inp]
    # the command line, so a change of flags rebuilds
    stamp = obj + ".cmd"
    cmdline = " ".join([CXX] + flags)
    same_flags = os.path.exists(stamp) and open(stamp).read() == cmdline
    if same_flags and os.path.exists(obj) and all(os.path.getmtime(obj) >= os.path.getmtime(d) for d in deps) \
            and not os.environ.get("PORT_FORCE"):
        dep = obj + ".d"
        if src.endswith(".s") or (os.path.exists(dep) and dep_fresh(dep, obj)):
            return rel, obj, 0, ""
    nice = ["nice", "-n", "10"] if shutil.which("nice") else []
    if src.endswith(".s"):
        cmd = nice + [CXX] + flags + ["-c", inp, "-o", obj]
    else:
        cmd = nice + [CXX] + flags + ["-MMD", "-MF", obj + ".d", "-c", inp, "-o", obj]
    p = subprocess.run(cmd, capture_output=True, text=True)
    if p.returncode == 0:
        with open(stamp, "w") as f:
            f.write(cmdline)
    return rel, obj, p.returncode, p.stderr


def dep_fresh(dep, obj):
    t = os.path.getmtime(obj)
    text = open(dep).read().replace("\\\n", " ")
    for path in text.split(":", 1)[1].split():
        if os.path.exists(path) and os.path.getmtime(path) > t:
            return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-j", type=int, default=8)
    ap.add_argument("--keep-going", action="store_true")
    ap.add_argument("--only", nargs="*", help="TU file names (e.g. Debug.cpp)")
    ap.add_argument("--no-link", action="store_true")
    ap.add_argument("--asan", action="store_true")
    ap.add_argument("--target", choices=["linux", "macos", "windows"], default=host_target())
    ap.add_argument("--arch", choices=["x86_64", "arm64"], default="x86_64",
                    help="macOS only: arm64 builds natively for Apple silicon (build/port-macos-arm64)")
    args = ap.parse_args()
    if args.arch == "arm64" and args.target != "macos":
        sys.exit("--arch arm64: macOS only")
    global OUT, VU_PACKETS, HOST_LIBS
    HOST_LIBS = args.target == "linux" or args.arch == "arm64"
    extra = []
    prefixes = dep_prefixes(args.target)
    if args.target != "linux":
        if args.asan:
            sys.exit("--asan: Linux only")
        configure(args.target, args.arch)
        VU_PACKETS = os.path.join(OUT, "gen/vupackets.s")
    if args.asan:
        OUT = os.path.join(ROOT, "build/port-asan")
        extra = ["-fsanitize=address", "-fno-omit-frame-pointer"]
        GAME_FLAGS.extend(extra)
        PORT_FLAGS.extend(extra)

    subprocess.run([sys.executable, os.path.join(ROOT, "tools/port/rewrite.py")], check=True)
    subprocess.run([sys.executable, os.path.join(ROOT, "tools/port/vu.py"), "gen", VU_PACKETS, VU_FORMAT], check=True)

    ff = ffmpeg_flags(args.target, prefixes)
    if ff:
        PORT_FLAGS.extend(ff[0])
        if args.target == "windows":
            # DLLs, by their import libraries: with SDL3, before -Bstatic
            at = LIBS.index("-lSDL3") + 1
            LIBS[at:at] = ff[1]
        else:
            LIBS.extend(ff[1])
    else:
        print("warning: libavcodec/libavutil not found (or PORT_NO_FFMPEG=1): "
              "building with movies stubbed (two black frames each); install ffmpeg's "
              "development files for movie playback (ports/pc/README.md)", file=sys.stderr)
        PORT_FLAGS.append("-DPORT_NO_FFMPEG")

    jobs = [(s, GAME_FLAGS, True) for s in game_sources()]
    jobs += [(s, GAME_FLAGS, False) for s in port_game_sources()]
    # PORT_O1="a.cpp b.cpp": build those ports/pc/ files at -O1 (bisecting)
    o1 = set(os.environ.get("PORT_O1", "").split())
    for src in port_sources():
        flags = [f if f != "-O2" or os.path.basename(src) not in o1 else "-O1" for f in PORT_FLAGS]
        if os.path.basename(src) == "heapguard.cpp":
            # it defines malloc & co: no builtin assumptions about them
            flags = flags + ["-fno-builtin"]
        jobs.append((src, flags, False))
    jobs += [(VU_PACKETS, ASM_FLAGS, False)]
    if args.only:
        jobs = [j for j in jobs if os.path.basename(j[0]) in args.only]

    objs, failed = [], []
    with concurrent.futures.ThreadPoolExecutor(args.j) as ex:
        for rel, obj, rc, err in ex.map(lambda j: compile_one(*j), jobs):
            if rc:
                failed.append(rel)
                nerr = err.count("error:")
                print(f"FAIL {rel} ({nerr} errors)")
                if not args.keep_going or args.only:
                    sys.stderr.write(err)
            else:
                objs.append(obj)
    print(f"compiled {len(objs)} of {len(jobs)}")
    if failed:
        return 1
    if args.no_link or args.only:
        return 0
    # Mach-O runs static constructors in link order (a constructor priority
    # only sorts within its file): the memory map (hw.cpp's HwInit), then the
    # VU packets' relocation, before every game TU's.
    first = ["ports/pc/hw.cpp.o", "ports/pc/sdk/microcode.cpp.o"]
    objs.sort(key=lambda o: next((i for i, f in enumerate(first) if o.endswith(f)), len(first)))
    exe = os.path.join(OUT, EXE)
    # PORT_LDFLAGS: more linker arguments
    more = os.environ.get("PORT_LDFLAGS", "").split()
    p = subprocess.run([CXX] + LINK_FLAGS + more + ["-o", exe] + extra + objs + LIBS, capture_output=True, text=True)
    if p.returncode:
        sys.stderr.write(p.stderr)
        return 1
    print(f"linked {os.path.relpath(exe, ROOT)}")
    if args.target == "windows":
        # the launcher and its notes, for handing the folder to someone
        for f in sorted(glob.glob(os.path.join(ROOT, "ports/pc/windows/*"))):
            shutil.copy2(f, os.path.join(OUT, os.path.basename(f)))
        licence = os.path.join(ROOT, "build/deps/windows/SDL3-LICENSE.txt")
        if os.path.exists(licence):
            shutil.copy2(licence, os.path.join(OUT, "SDL3-LICENSE.txt"))
    app = os.path.join(OUT, "Splashdown.app") if args.target == "macos" else None
    if app:
        # Splashdown.app: a launcher script (it finds the disc image and
        # keeps a log), the game and, below, SDL3 beside it
        shutil.rmtree(app, ignore_errors=True)
        os.makedirs(os.path.join(app, "Contents/MacOS"))
        shutil.copy2(os.path.join(ROOT, "ports/pc/macos/Info.plist"), os.path.join(app, "Contents/Info.plist"))
        shutil.copy2(os.path.join(ROOT, "ports/pc/macos/launch"), os.path.join(app, "Contents/MacOS/launch"))
        shutil.copy2(exe, os.path.join(app, "Contents/MacOS/splashdown"))
    # SDL3's shared library beside the executable, where the loader looks
    # first (Windows) or its install name may point (macOS: @loader_path, @rpath)
    for prefix in prefixes:
        for rel in ("lib/libSDL3.0.dylib",) if args.target == "macos" else \
                ("bin/SDL3.dll", "bin/avcodec-*.dll", "bin/avutil-*.dll", "bin/swresample-*.dll") \
                if args.target == "windows" else ():
            for lib in glob.glob(os.path.join(prefix, rel)):
                shutil.copy2(lib, os.path.join(OUT, os.path.basename(lib)))
                print(f"copied {os.path.basename(lib)}")
                if app:
                    shutil.copy2(lib, os.path.join(app, "Contents/MacOS", os.path.basename(lib)))
    if app:
        # ad-hoc signature: the bundle as a whole, so Finder will open it
        subprocess.run(["codesign", "--force", "--deep", "-s", "-", app], capture_output=True)
        print(f"bundled {os.path.relpath(app, ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
