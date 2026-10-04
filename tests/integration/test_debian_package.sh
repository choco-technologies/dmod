#!/usr/bin/env bash
# Validate a native .deb without installing it or requiring root.
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: $0 package.deb [example_app.dmf]" >&2
    exit 1
fi

package=$(realpath "$1")
root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT
dpkg-deb --extract "$package" "$root"

[[ $(dpkg-deb --field "$package" Package) == dmod ]]
[[ $(dpkg-deb --field "$package" Architecture) == "$(dpkg --print-architecture)" ]]
dependencies=$(dpkg-deb --field "$package" Depends)
[[ $dependencies == *unzip* ]]
[[ $dependencies == *libc6* ]]

tools=(dmf-get dmf-man dmod_loader mkdmrpkg todmd todmfc todmm todmp whereisdmf)
expected=$(printf '%s\n' "${tools[@]}" | sort)
actual=$(find "$root/usr/bin" -mindepth 1 -maxdepth 1 -printf '%f\n' | sort)
[[ $actual == "$expected" ]]
[[ ! -e "$root/usr/local" && ! -e "$root/usr/include" && ! -e "$root/usr/lib" ]]
[[ -s "$root/usr/share/doc/dmod/copyright" ]]
[[ -s "$root/usr/share/doc/dmod/debian-package.md" ]]

for tool in "${tools[@]}"; do
    [[ -x "$root/usr/bin/$tool" ]]
    "$root/usr/bin/$tool" --help > "$root/$tool.help" 2>&1
    grep -q 'Usage:' "$root/$tool.help"
done

if [[ $# == 2 ]]; then
    module=$(realpath "$2")
    # Copy the application and its library into the installed loader's default
    # search directory, without relying on build paths or environment overrides.
    mkdir "$root/dmf"
    cp "$module" "$(dirname "$module")/dmodex.dmf" "$root/dmf/"
    unset DMOD_REPO_PATHS DMOD_DMF_DIR DMOD_DMFC_DIR
    # Run away from the source/build tree, using only packaged executables.
    cd "$root"
    ./usr/bin/dmod_loader example_app
    ./usr/bin/todmfc "$module" example_app.dmfc
    ./usr/bin/dmod_loader "$root/example_app.dmfc"
fi

echo "Debian package contents, metadata and executable smoke tests passed."
