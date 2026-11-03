#!/bin/sh
# Build talkd with sanitizers against the test-only tcpd shim and run the
# loopback scenarios.  ASan/UBSan runs everywhere; the MemorySanitizer
# scenario needs clang and is skipped without it.  usage: tests/run.sh
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$here/..
out=${TMPDIR:-/tmp}/talkd-test.$$
trap 'rm -rf "$out"' EXIT
mkdir -p "$out"
export TALKD_MAX_LIFE=2 TALKD_MAX_TABLE=16
defs="-D_GNU_SOURCE -DMAX_LIFE=$TALKD_MAX_LIFE
    -DMAX_TABLE_ENTRIES=$TALKD_MAX_TABLE -D_PATH_VAR_LOG=\"$out/\""
src="$top/talkd/*.c $here/tcpd_stub.c"
touch "$out/talkd.log" "$out/talkd.packets"
rc=0

${CC:-cc} -g -O0 -Wall -fsanitize=address,undefined \
    -fno-sanitize-recover=undefined $defs -I"$here" -I"$top/include" \
    -o "$out/talkd-asan" $src
python3 -I "$here/probe.py" "$out/talkd-asan" expire cap || rc=1

if command -v clang >/dev/null; then
    clang -g -O0 -fsanitize=memory -fno-omit-frame-pointer $defs \
        -I"$here" -I"$top/include" -o "$out/talkd-msan" $src
    python3 -I "$here/probe.py" "$out/talkd-msan" reply_clean || rc=1
else
    echo "SKIP reply_clean (needs clang for MemorySanitizer)"
fi
exit $rc
