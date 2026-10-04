#!/usr/bin/env bash
set -euo pipefail
prefix=$1
destination=$2
profile=$3
mkdir -p "$destination"
for tool in todmv todmvi todmvf; do
    "$prefix/bin/dmf-get" "$tool" -t "$profile" --type dmf -y -o "$destination/$tool"
done
"$prefix/bin/dmf-get" dmimg_png -t "$profile" --type dmf -y -o "$destination/todmvi"
