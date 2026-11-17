#!/bin/sh
# Host tests of the Block Shooter port. Needs a C compiler; for the 68k
# half, m68k-linux-gnu-gcc and qemu-m68k (skipped, and said so, without them).
#   sh tests/host/test_all.sh [WORKDIR]
# 1. amiga/bs_core.c, the rules: their checks on the PC and on 68k under
#    qemu-m68k -cpu m68040; the two must print the same.
# 2. tests/host/test_*.py (the PNG reader, the LhA packer).
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
W=${1:-"${TMPDIR:-/tmp}/amigablockshooter-test"}
mkdir -p "$W"
cc -O2 -std=c99 -Wall -I"$ROOT/amiga" -o "$W/test_bs_core-pc" "$HERE/test_bs_core.c" "$ROOT/amiga/bs_core.c"
"$W/test_bs_core-pc" > "$W/bs_core-pc.txt" || { cat "$W/bs_core-pc.txt"; exit 1; }
cat "$W/bs_core-pc.txt"
if command -v m68k-linux-gnu-gcc >/dev/null && command -v qemu-m68k >/dev/null; then
    m68k-linux-gnu-gcc -m68040 -O2 -static -I"$ROOT/amiga" -o "$W/test_bs_core-68k" \
        "$HERE/test_bs_core.c" "$ROOT/amiga/bs_core.c"
    qemu-m68k -cpu m68040 "$W/test_bs_core-68k" > "$W/bs_core-68k.txt" || { cat "$W/bs_core-68k.txt"; exit 1; }
    if diff -u "$W/bs_core-pc.txt" "$W/bs_core-68k.txt"; then
        echo "PASS rules: PC and 68k the same"
    else
        echo "FAIL rules: PC and 68k differ"; exit 1
    fi
else
    echo "SKIP rules on 68k: no m68k-linux-gnu-gcc or qemu-m68k"
fi
for t in "$HERE"/test_*.py; do
    python3 "$t"
done
echo "ALL PASS"
