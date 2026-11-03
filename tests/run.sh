#!/bin/sh
# Build talkd with ASan/UBSan against the test-only tcpd shim and run the
# loopback scenarios.  usage: tests/run.sh [scenario...]
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$here/..
out=${TMPDIR:-/tmp}/talkd-test.$$
trap 'rm -rf "$out"' EXIT
mkdir -p "$out"
export TALKD_MAX_LIFE=2 TALKD_MAX_TABLE=16
${CC:-cc} -g -O0 -Wall -fsanitize=address,undefined \
    -fno-sanitize-recover=undefined -D_GNU_SOURCE \
    -DMAX_LIFE=$TALKD_MAX_LIFE -DMAX_TABLE_ENTRIES=$TALKD_MAX_TABLE \
    -D_PATH_VAR_LOG="\"$out/\"" -I"$here" -I"$top/include" \
    -o "$out/talkd" "$top"/talkd/*.c "$here/tcpd_stub.c"
touch "$out/talkd.log" "$out/talkd.packets"
[ $# -gt 0 ] || set -- expire cap
exec python3 -I "$here/probe.py" "$out/talkd" "$@"
