#!/usr/bin/env bash
set -euo pipefail
if [[ $# != 2 ]]; then
    echo "Usage: $0 dmod.deb dmod-dev.deb" >&2
    exit 1
fi
runtime_version=$(dpkg-deb --field "$1" Version)
[[ $(dpkg-deb --field "$2" Package) == dmod-dev ]]
[[ $(dpkg-deb --field "$2" Architecture) == all ]]
[[ $(dpkg-deb --field "$2" Depends) == *"dmod (= $runtime_version)"* ]]
root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT
dpkg-deb --extract "$2" "$root"
for file in include/dmod/dmod.h include/dmod/Dmod.hpp \
    share/cmake/dmod/dmodConfig.cmake share/cmake/dmod/dmodConfigVersion.cmake \
    share/dmod/scripts/module.ld share/dmod/scripts/api.h.in \
    share/dmod/configs/arch/armv7/cortex-m7/tools-cfg.cmake \
    share/dmod/configs/arch/x86_64/tools-cfg.cmake \
    share/dmod/src/module/dmod_module.c share/dmod/src/module/dmod_test_main.c \
    share/doc/dmod-dev/copyright share/doc/dmod-dev/copyright.FastLZ; do
    test -s "$root/usr/$file"
done
test ! -e "$root/usr/bin"
test -z "$(find "$root" -type f \( -name '*.o' -o -name '*.a' -o -name '*.so*' \) -print)"
echo "Development package metadata and contents passed"
