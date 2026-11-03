#!/usr/bin/env bash
set -euo pipefail
prefix=$1
work=$2
profile=$3
checkout=${4:-}

for kind in application library port; do
    name="generated_$kind"
    source="$work/$kind"
    build="$work/build-$kind"
    args=(-DCMAKE_PREFIX_PATH="$prefix"
        -DDMOD_DMF_DIR="$build/dmf" -DDMOD_DMFC_DIR="$build/dmfc"
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON -DDMOD_GENERATE_MAKEFILES=OFF)
    # The SDK application also exercises the documented default configure
    # without selecting a profile. Libraries and ports exercise profile loading.
    if [[ $kind != application || -n $checkout ]]; then
        args+=(-DDMOD_TOOLS_NAME="$profile")
    fi
    if [[ -n $checkout ]]; then
        # A local checkout must win even with dmod-dev available in this prefix.
        args+=(-DDMOD_DIR="$checkout")
    fi
    cmake -S "$source" -B "$build" "${args[@]}"
    cmake --build "$build" --parallel 2
    for module in "$name" "test_$name"; do
        test -s "$build/dmf/$module.dmf"
        test -s "$build/dmfc/$module.dmfc"
    done
    test -s "$build/packages/$name/$name.dmr"
    if [[ $kind == port ]]; then
        test -s "$build/dmf/${name}_port.dmf"
        test -s "$build/packages/${name}_port/${name}_port.dmf"
    fi
    if [[ -z $checkout ]]; then
        test -s "$build/dmod-sdk/dmod-config.h"
        test ! -e "$build/_deps/dmod-build"
    else
        test -s "$build/_deps/dmod-build/dmod-config.h"
        test ! -e "$build/dmod-sdk"
    fi
    ctest --test-dir "$build" --output-on-failure
    if [[ $kind == application ]]; then
        "$prefix/bin/dmod_loader" "$build/dmf/$name.dmf"
    fi
done

if [[ -n $checkout ]]; then
    # Keep the Git/FetchContent workflow selectable with an installed SDK too.
    # Supply a local FetchContent source override so this test needs no clone.
    build="$work/build-fetchcontent"
    cmake -S "$work/application" -B "$build" \
        -DCMAKE_PREFIX_PATH="$prefix" -DDMOD_TOOLS_NAME="$profile" \
        -DDMOD_USE_INSTALLED_SDK=OFF -DFETCHCONTENT_SOURCE_DIR_DMOD="$checkout" \
        -DDMOD_DMF_DIR="$build/dmf" -DDMOD_DMFC_DIR="$build/dmfc" \
        -DDMOD_GENERATE_MAKEFILES=OFF
    cmake --build "$build" --parallel 2
    test -s "$build/_deps/dmod-build/dmod-config.h"
    test -s "$build/dmf/generated_application.dmf"
    # Exercise the automatic fallback when find_package cannot find an SDK.
    # Prepopulate FetchContent with a symlink to avoid downloading a checkout.
    build="$work/build-no-sdk"
    mkdir -p "$build/_deps"
    ln -s "$checkout" "$build/_deps/dmod-src"
    cmake -S "$work/application" -B "$build" \
        -DCMAKE_PREFIX_PATH="$prefix" -DDMOD_TOOLS_NAME="$profile" \
        -DCMAKE_DISABLE_FIND_PACKAGE_dmod=ON -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
        -DDMOD_DMF_DIR="$build/dmf" -DDMOD_DMFC_DIR="$build/dmfc" \
        -DDMOD_GENERATE_MAKEFILES=OFF
    cmake --build "$build" --parallel 2
    test -s "$build/_deps/dmod-build/dmod-config.h"
    test -s "$build/dmf/generated_application.dmf"
fi

echo "Generated application, library and port tests passed."
