#!/usr/bin/env bash
# Requires bubblewrap; on systems restricting user namespaces, invoke via sudo.
set -euo pipefail
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 installed-prefix [--cross | --cross-profile name] [CMake options...]" >&2
    exit 1
fi
prefix=$(realpath "$1")
shift
checkout=$(realpath "$(dirname "$0")/../..")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp -r "$checkout/tests/integration/sdk" "$work/source"
if [[ -n ${DMOD_SDK_TEST_ASSETS:-} ]]; then
    cp -r "$DMOD_SDK_TEST_ASSETS" "$work/assets-tools"
    cp /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf "$work/source/assets/fixtures/font.ttf"
    python3 - "$work/source/assets/fixtures/pixel.png" <<'PY'
import struct, sys, zlib
def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
data = b'\x89PNG\r\n\x1a\n'
data += chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 2, 0, 0, 0))
data += chunk(b'IDAT', zlib.compress(b'\x00\xff\x00\x00')) + chunk(b'IEND', b'')
open(sys.argv[1], 'wb').write(data)
PY
fi
mkdir "$work/home"
cd "$work"

# No checkout, no network, a read-only SDK and source fixture, and no inherited
# DMOD variables/tool caches. Only the consumer build directory is writable.
bwrap --die-with-parent --unshare-net --ro-bind / / --tmpfs /tmp \
    --tmpfs "$checkout" --bind "$work" "$work" \
    --ro-bind "$work/source" "$work/source" --ro-bind "$prefix" "$prefix" \
    --dev /dev --proc /proc --chdir "$work" \
    /usr/bin/env -i PATH="$prefix/bin:$PATH" HOME="$work/home" \
    /bin/bash "$work/source/run.sh" "$prefix" "$work" "$@"
