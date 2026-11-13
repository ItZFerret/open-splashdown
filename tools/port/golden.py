#!/usr/bin/env python3
"""Golden-image regression check for the PC port.

A deterministic headless run (PORT_DETERMINISTIC: virtual time, so the game
does the same thing frame for frame whatever the host's speed) through the
front end (memory-card prompt, main menu, Select Racer, Arcade setup) and
~40 s of a Hawaii race with a fixed PORT_INPUT script, a screenshot every
--every finished frames. `record` saves the pictures as the reference in
build/port/golden/; `check` runs again into build/port/golden-check/ and
compares pixel for pixel (exact; the PNGs are decoded only when the bytes
differ), listing every frame that differs with its pixel count and largest
channel difference, and writing <frame>_diff.png (differing pixels in red).

usage: tools/port/golden.py record|check [--audio] [--quick] [--frames N]
                                          [--every N] [--exe PATH]

--quick stops at frame 700 (the front end only, ~30 s at today's speed).
No memory card (PORT_NO_MEMCARD=1: the prompt is answered "start without
it"), movies stubbed (PORT_NO_MOVIES=1). The default session runs with the sound driver off (PORT_NO_AUDIO=1);
--audio runs it with the driver on (SDL's dummy device, mixed on the game
thread) against a reference of its own in build/port/golden-audio/
(golden-audio-check/), and also compares the mix (PORT_AUDIO_WAV) by SHA-256.
Exit status 0 when every frame (and the mix) matches.
"""
import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import time
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GOLDEN = os.path.join(ROOT, "build/port/golden")
CHECK = os.path.join(ROOT, "build/port/golden-check")
GOLDEN_AUDIO = os.path.join(ROOT, "build/port/golden-audio")
CHECK_AUDIO = os.path.join(ROOT, "build/port/golden-audio-check")

# Front end (memory-card prompt, Arcade, Select Racer,
# Arcade setup, start), then throttle held through the race with a few turns.
INPUT = ("150:cross,480:cross,560:cross,640:cross,"
         "760:cross/9000,2500:lleft/60,3500:lright/80,5000:lleft/100,7000:lright/60")
FRAMES = 9000
EVERY = 100


def run(outdir, frames, every, exe, audio=False):
    if os.path.isdir(outdir):
        shutil.rmtree(outdir)
    os.makedirs(outdir)
    env = dict(os.environ)
    for k in list(env):
        if k.startswith("PORT_"):
            del env[k]
    env.update({
        "PORT_DETERMINISTIC": "2",
        # No memory card: the reference starts without one (the prompt).
        "PORT_NO_MEMCARD": "1",
        # Movies as two black frames each: both references were recorded
        # before movie playback (the logos and the intro take ~25 s of
        # boot now).
        "PORT_NO_MOVIES": "1",
        "PORT_HEADLESS": "1",
        "PORT_INPUT": INPUT,
        "PORT_SCREENSHOT": os.path.join(outdir, "f.png"),
        "PORT_SCREENSHOT_EVERY": str(every),
        "PORT_EXIT_FRAME": str(frames),
    })
    if audio:
        # The driver's replies steer the game's sound code, whose rand()
        # calls the race's particles share: a reference of its own.
        env["SDL_AUDIO_DRIVER"] = "dummy"
        env["PORT_AUDIO_WAV"] = os.path.join(outdir, "mix.wav")
    else:
        # The first reference was recorded before the sound driver
        #.
        env["PORT_NO_AUDIO"] = "1"
    for k in ("PORT_GS_THREADS", "PORT_VU1_NOTIMING"):
        if k in os.environ:
            env[k] = os.environ[k]
    log = os.path.join(outdir, "run.log")
    t = time.time()
    with open(log, "w") as f:
        # Linux: below the desktop's priority, on the first 16 CPUs
        prefix = [t for t in (["nice", "-n", "5"], ["taskset", "-c", "0-15"]) if shutil.which(t[0])]
        p = subprocess.run([a for t in prefix for a in t] + [exe], env=env, stdout=f, stderr=subprocess.STDOUT)
    secs = time.time() - t
    print(f"run: {secs:.0f} s, exit {p.returncode}, log {os.path.relpath(log, ROOT)}")
    return p.returncode


def frames_in(d):
    out = {}
    for n in os.listdir(d):
        if n.startswith("f_") and n.endswith(".png") and "_diff" not in n:
            out[int(n[2:-4])] = os.path.join(d, n)
    return out


def decode_png(path):
    """Our own PNGs: 8-bit RGB, filter 0 on every row."""
    data = open(path, "rb").read()
    pos, idat, w, h = 8, b"", 0, 0
    while pos < len(data):
        n, typ = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", body[:8])
        elif typ == b"IDAT":
            idat += body
        pos += 12 + n
    raw = zlib.decompress(idat)
    stride = w * 3 + 1
    return w, h, b"".join(raw[y * stride + 1:(y + 1) * stride] for y in range(h))


def write_png(path, w, h, rgb):
    def chunk(t, b):
        c = t + b
        return struct.pack(">I", len(b)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)
    raw = b"".join(b"\0" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def compare(ref, new, diff_path):
    if open(ref, "rb").read() == open(new, "rb").read():
        return None
    w1, h1, a = decode_png(ref)
    w2, h2, b = decode_png(new)
    if (w1, h1) != (w2, h2):
        return f"size {w1}x{h1} vs {w2}x{h2}"
    npix, worst = 0, 0
    out = bytearray(len(a) // 3 * 3)
    for i in range(0, len(a), 3):
        d = max(abs(a[i] - b[i]), abs(a[i + 1] - b[i + 1]), abs(a[i + 2] - b[i + 2]))
        if d:
            npix += 1
            worst = max(worst, d)
            out[i:i + 3] = b"\xff\0\0"
        else:
            g = (a[i] + a[i + 1] + a[i + 2]) // 6
            out[i:i + 3] = bytes((g, g, g))
    if not npix:
        return None
    write_png(diff_path, w1, h1, bytes(out))
    return f"{npix} pixels differ (max {worst})"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["record", "check"])
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--audio", action="store_true")
    ap.add_argument("--frames", type=int)
    ap.add_argument("--every", type=int, default=EVERY)
    host = {"darwin": "build/port-macos/splashdown", "win32": "build/port-windows/splashdown.exe"}
    ap.add_argument("--exe", default=os.path.join(ROOT, host.get(sys.platform, "build/port/splashdown")))
    args = ap.parse_args()
    frames = args.frames or (700 if args.quick else FRAMES)
    golden, check = (GOLDEN_AUDIO, CHECK_AUDIO) if args.audio else (GOLDEN, CHECK)
    if args.mode == "record":
        rc = run(golden, frames, args.every, args.exe, args.audio)
        n = len(frames_in(golden))
        print(f"recorded {n} frames in {os.path.relpath(golden, ROOT)}")
        if args.audio:
            h = wav_hash(golden, frames)
            print(f"mix: {h}")
        return 0 if rc == 0 and n else 1
    rc = run(check, frames, args.every, args.exe, args.audio)
    ref, new = frames_in(golden), frames_in(check)
    want = sorted(f for f in ref if f <= frames)
    bad = 0
    for f in want:
        if f not in new:
            print(f"frame {f}: missing")
            bad += 1
            continue
        r = compare(ref[f], new[f], os.path.join(check, f"f_{f}_diff.png"))
        if r:
            print(f"frame {f}: {r}")
            bad += 1
    print(f"golden: {len(want) - bad} of {len(want)} frames match" + ("" if bad else " (OK)"))
    if args.audio:
        want_h = open(os.path.join(golden, f"mix_{frames}.sha256")).read().strip() \
            if os.path.exists(os.path.join(golden, f"mix_{frames}.sha256")) else None
        got_h = wav_hash(check, frames, save=False)
        if want_h is None:
            print("mix: no reference for this length")
        elif got_h == want_h:
            print("mix: matches (OK)")
        else:
            print(f"mix: differs ({got_h} vs {want_h})")
            bad += 1
    return 0 if bad == 0 and rc == 0 else 1


def wav_hash(d, frames, save=True):
    """SHA-256 of the run's mix; the WAV itself is removed (~1 MB a second)."""
    path = os.path.join(d, "mix.wav")
    if not os.path.exists(path):
        return None
    h = hashlib.sha256(open(path, "rb").read()).hexdigest()
    os.remove(path)
    if save:
        with open(os.path.join(d, f"mix_{frames}.sha256"), "w") as f:
            f.write(h + "\n")
    return h


if __name__ == "__main__":
    sys.exit(main())
