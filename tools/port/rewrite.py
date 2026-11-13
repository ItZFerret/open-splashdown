#!/usr/bin/env python3
"""Mirror src/ and include/ into build/port/tree/ with inline asm made portable.

Every `asm(...)` / `asm volatile(...)` statement becomes a port_asm() call
(ports/pc/include/port_asm.h) that ports/pc/ee/asm_interp.cpp interprets. The
statement keeps its line count, so diagnostics and __LINE__ point at the
original source. Whole-function `asm int f(...) { ... }` bodies are left
alone: their source guards them with SPLASHDOWN_PC.

Only files whose source is newer than their copy are rewritten.

usage: tools/port/rewrite.py [--out build/port/tree] [--check FILE]
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TREES = ("src", "include")
EXTS = (".c", ".cpp", ".h", ".hpp", ".inc")

ASM_RE = re.compile(r"\basm\b(\s+volatile\b)?\s*\(")


def skip_string(s, i):
    """s[i] is a quote; return the index after the closing quote."""
    q = s[i]
    i += 1
    while i < len(s) and s[i] != q:
        i += 2 if s[i] == "\\" else 1
    return i + 1


def find_close(s, i):
    """s[i-1] is '('; return the index of the matching ')'."""
    depth = 1
    while i < len(s):
        c = s[i]
        if c in "\"'":
            i = skip_string(s, i)
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unbalanced asm(")


def split_top(s, sep):
    """Split on sep outside strings and brackets."""
    parts, depth, start, i = [], 0, 0, 0
    while i < len(s):
        c = s[i]
        if c in "\"'":
            i = skip_string(s, i)
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == sep and depth == 0:
            parts.append(s[start:i])
            start = i + 1
        i += 1
    parts.append(s[start:])
    return parts


def c_string(lits):
    """Concatenate adjacent string literals into one C string literal."""
    body = "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', lits))
    return '"' + body + '"'


MAKERS = {
    "r": "port_in_r",
    "f": "port_in_f",
    "=r": "port_out_r",
    "=&r": "port_out_r",
    "=f": "port_out_f",
    "=&f": "port_out_f",
    "+r": "port_inout_r",
}


def convert(body, where):
    sections = split_top(body, ":")
    text = c_string(sections[0])
    operands = []
    for sec in sections[1:3]:
        for op in split_top(sec, ","):
            op = op.strip()
            if not op:
                continue
            m = re.match(r'"([^"]*)"\s*\((.*)\)$', op, re.S)
            if not m or m.group(1) not in MAKERS:
                raise ValueError(f"{where}: unsupported asm operand {op!r}")
            operands.append(f"{MAKERS[m.group(1)]}({' '.join(m.group(2).split())})")
    if operands:
        args = "PortAsmArg _paa[] = {" + ", ".join(operands) + "}; "
        call = f"port_asm(&_pap, {text}, _paa, {len(operands)});"
    else:
        args = ""
        call = f"port_asm(&_pap, {text}, 0, 0);"
    return "do { static PortAsmProg _pap; " + args + call + " } while (0)"


def blank_comments(s):
    """Return s with comments replaced by spaces (same length), strings kept."""
    out = list(s)
    i = 0
    while i < len(s):
        c = s[i]
        if c in "\"'":
            i = skip_string(s, i)
        elif s.startswith("//", i):
            j = s.find("\n", i)
            j = len(s) if j < 0 else j
            out[i:j] = " " * (j - i)
            i = j
        elif s.startswith("/*", i):
            j = s.find("*/", i + 2)
            j = len(s) if j < 0 else j + 2
            out[i:j] = [ch if ch == "\n" else " " for ch in s[i:j]]
            i = j
        else:
            i += 1
    return "".join(out)


# A hardware address cast to a pointer, `(volatile int *)0x10003000`: the EE's
# registers (0x1000xxxx), the GS's (0x1200xxxx), the scratchpad (0x7000xxxx).
# ... or named first: `#define VIF1_STAT 0x10003C00`, used as `*(T *)VIF1_STAT`.
# (The same numbers elsewhere are VIF codes and DMA tag words: left alone.)
HW_RE = re.compile(r"(\*\s*\)\s*|#define[ \t]+[A-Z][A-Z0-9_]*[ \t]+)(0x(?:1[02]00|7000)[0-9A-Fa-f]{4})\b")


def hardware_addresses(src):
    """Hardware addresses as pointers get the PC's base (PORT_HW,
    ports/pc/include/port_ptr32.h: 0 where game memory lies below 4 GB)."""
    scan = blank_comments(src)
    out, last = [], 0
    for m in HW_RE.finditer(scan):
        out.append(src[last:m.start(2)])
        out.append("PORT_HW(" + m.group(2) + ")")
        last = m.end(2)
    out.append(src[last:])
    return "".join(out)


def rewrite(src, where):
    src = hardware_addresses(src)
    scan = blank_comments(src)
    out, last = [], 0
    for m in ASM_RE.finditer(scan):
        if m.start() < last:
            continue
        open_at = m.end()
        close = find_close(scan, open_at)
        body = src[open_at:close]
        lineno = src.count("\n", 0, m.start()) + 1
        new = convert(body, f"{where}:{lineno}")
        newlines = body.count("\n")
        # Inside a macro the lines end in backslashes; keep them.
        line_start = src.rfind("\n", 0, m.start()) + 1
        in_macro = src[line_start:m.start()].lstrip().startswith("#") or "\\\n" in body
        tail = (" \\\n" if in_macro else "\n") * newlines
        out.append(src[last:m.start()])
        out.append(new + tail)
        last = close + 1
    out.append(src[last:])
    return "".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "build/port/tree"))
    ap.add_argument("--check", help="rewrite one file to stdout")
    args = ap.parse_args()
    if args.check:
        sys.stdout.write(rewrite(open(args.check, errors="surrogateescape").read(), args.check))
        return
    changed = 0
    for tree in TREES:
        for dirpath, _, files in os.walk(os.path.join(ROOT, tree)):
            for name in files:
                if not name.endswith(EXTS):
                    continue
                src = os.path.join(dirpath, name)
                rel = os.path.relpath(src, ROOT)
                dst = os.path.join(args.out, rel)
                if os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src):
                    continue
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                with open(src, errors="surrogateescape") as f:
                    text = f.read()
                with open(dst, "w", errors="surrogateescape") as f:
                    f.write(rewrite(text, rel))
                changed += 1
    print(f"rewrite: {changed} files")


if __name__ == "__main__":
    main()
