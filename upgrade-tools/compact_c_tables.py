#!/usr/bin/env python3
"""Whitespace-only compaction of generated C tables (nir_opt_algebraic.c):
- runs of lines holding a single integer initializer ("   24,") are merged
  into lines of up to 32 values
- multi-line union initializers of nir_search_value_union arrays
  ("   { .expression = {" ... "   } },") are joined into one line each
- "{ search, replace, cond }," transform entries are packed 8 per line
- the generator's "/* replaceN -> M in the cache */" debug comments and
  repeated blank lines are dropped

usage: compact_c_tables.py [--blank-lines-only] FILE [FILE...]   (rewrites in place)
  --blank-lines-only: only drop whitespace-only lines (nir_constant_expressions.c)
"""
import re
import sys

NUM = re.compile(r"^(\s*)(-?(?:0x[0-9a-fA-F]+|\d+)),\s*$")
VALUE_START = re.compile(r"^   \{ \.(?:variable|constant|expression) = \{\s*$")
VALUE_END = "   } },"
CACHE_COMMENT = re.compile(r"^\s*/\* \w+ -> \d+ in the cache \*/\s*$")
TRANSFORM = re.compile(r"^(\s*)(\{ \d+, \d+, \d+ \}),\s*$")
PER_LINE = 32
TRANSFORMS_PER_LINE = 8


def compact(lines):
    out, run, indent = [], [], ""
    value = None
    per = PER_LINE

    def flush():
        for i in range(0, len(run), per):
            out.append(indent + ", ".join(run[i:i + per]) + ",\n")
        run.clear()

    for line in lines:
        if value is not None:
            value.append(line.strip())
            if line.rstrip("\n") == VALUE_END:
                out.append("   " + " ".join(p for p in value if p) + "\n")
                value = None
            continue
        if CACHE_COMMENT.match(line):
            continue
        if VALUE_START.match(line):
            flush()
            value = [line.strip()]
            continue
        for rx, n in ((NUM, PER_LINE), (TRANSFORM, TRANSFORMS_PER_LINE)):
            m = rx.match(line)
            if m:
                if run and per != n:
                    flush()
                if not run:
                    indent = m.group(1)
                    per = n
                run.append(m.group(2))
                break
        if m:
            continue
        flush()
        if line.strip() == "" and out and out[-1].strip() == "":
            continue
        out.append(line)
    flush()
    assert value is None, "unterminated value initializer"
    return out


args = sys.argv[1:]
blank_only = "--blank-lines-only" in args
for path in [a for a in args if a != "--blank-lines-only"]:
    with open(path) as f:
        lines = f.readlines()
    if blank_only:
        lines = [l for l in lines if l.strip()]
    else:
        lines = compact(lines)
    with open(path, "w") as f:
        f.writelines(lines)
