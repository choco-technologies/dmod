#!/usr/bin/env bash
set -euo pipefail
prefix=$1
work=$2
shift 2
cross=false
package_args=()
if [[ $prefix != /usr ]]; then
    package_args+=("-DCMAKE_PREFIX_PATH=$prefix")
fi
if [[ ${1:-} == --cross ]]; then
    cross=true
    shift
    set -- "-DCMAKE_TOOLCHAIN_FILE=$work/source/arm-toolchain.cmake" "$@"
    package_args+=("-Ddmod_DIR=$prefix/share/cmake/dmod")
elif [[ ${1:-} == --cross-profile ]]; then
    cross=true
    profile=$2
    shift 2
    set -- "-DDMOD_TOOLS_NAME=$profile" "$@"
fi

cmake -S "$work/source" -B "$work/build" \
    "${package_args[@]}" "$@"
cmake --build "$work/build" --parallel 2
for module in sdk_math sdk_app sdk_test; do
    test -s "$work/build/dmf/$module.dmf"
    test -s "$work/build/dmf/$module.dmd"
    test -s "$work/build/dmfc/$module.dmfc"
done

python3 "$work/source/serve.py" "$work/build/packages" "$work/port" &
server=$!
trap 'kill "$server"; wait "$server" || true' EXIT
for attempt in {1..50}; do
    [[ -s "$work/port" ]] && break
    kill -0 "$server"
    sleep 0.1
done
printf 'sdk_math@0.1 http://127.0.0.1:%s/sdk_math.zip\n' "$(cat "$work/port")" > "$work/manifest.dmm"
cmake -S "$work/source/external" -B "$work/external-build" \
    "${package_args[@]}" \
    -DDMOD_DMM_URL="$work/manifest.dmm" "$@"
test -s "$work/external-build/dmf/inc/sdk_math/include/library.h"
test -s "$work/external-build/dmf/inc/sdk_math/include/sdk_math_defs.h"
cmake --build "$work/external-build" --parallel 2

if ! $cross; then
    export DMOD_REPO_PATHS="$work/build/dmf:$work/build/dmfc"
    "$prefix/bin/dmod_loader" "$work/build/dmf/sdk_app.dmf"
    "$prefix/bin/dmod_loader" "$work/build/dmfc/sdk_app.dmfc"
    "$prefix/bin/dmod_loader" "$work/build/dmf/sdk_test.dmf"
    "$prefix/bin/dmod_loader" "$work/external-build/dmf/sdk_external.dmf"
fi
if [[ -d "$work/assets-tools" ]]; then
    python3 "$work/source/assets/run.py" "$prefix" "$work" "${package_args[@]}" "$@"
fi
echo "Installed SDK test passed (cross=$cross)"
