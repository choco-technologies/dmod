#!/usr/bin/env bash
# Requires bubblewrap; on systems restricting user namespaces, invoke via sudo.
set -euo pipefail
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 installed-prefix [--cross] [CMake options...]" >&2
    exit 1
fi
prefix=$(realpath "$1")
shift
checkout=$(realpath "$(dirname "$0")/../..")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp -r "$checkout/tests/integration/sdk" "$work/source"
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
