#!/usr/bin/env bash
# SDK mode requires bubblewrap; invoke via sudo when user namespaces are restricted.
set -euo pipefail
if [[ $# -lt 1 || $# -gt 3 ]]; then
    echo "Usage: $0 installed-prefix [--source] [DMOD_TOOLS_NAME]" >&2
    exit 1
fi
prefix=$(realpath "$1")
mode=${2:---sdk}
profile=${3:-arch/x86_64}
if [[ $mode != --sdk && $mode != --source ]]; then
    echo "Expected --sdk or --source" >&2
    exit 1
fi
checkout=$(realpath "$(dirname "$0")/../..")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir "$work/home"
for kind in application library port; do
    args=(--name "generated_$kind" --path "$work/$kind" --skip-claude-sync)
    if [[ $kind == port ]]; then
        args+=(--type library --port --port-arch x86_64)
    else
        args+=(--type "$kind")
    fi
    bash "$checkout/scripts/new-module.sh" "${args[@]}"
done
cp "$checkout/tests/integration/generated-modules/run.sh" "$work/run.sh"

if [[ $mode == --source ]]; then
    PATH="$prefix/bin:$PATH" bash "$work/run.sh" "$prefix" "$work" "$profile" "$checkout"
else
    # Hide the checkout and network, and keep the SDK and generated sources
    # read-only. Only the consumer build directories and home are writable.
    bwrap --die-with-parent --unshare-net --ro-bind / / --tmpfs /tmp \
        --tmpfs "$checkout" --bind "$work" "$work" \
        --ro-bind "$prefix" "$prefix" \
        --ro-bind "$work/application" "$work/application" \
        --ro-bind "$work/library" "$work/library" \
        --ro-bind "$work/port" "$work/port" \
        --dev /dev --proc /proc --chdir "$work" \
        /usr/bin/env -i PATH="$prefix/bin:$PATH" HOME="$work/home" \
        /bin/bash "$work/run.sh" "$prefix" "$work" "$profile"
fi
