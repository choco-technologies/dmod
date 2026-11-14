#!/bin/bash
# new-module.sh --gui: an application with a page, an icon and a main() that claims a display; no placeholder is left, the
# CMakeLists.txt has the wiring (the conversion is dmod's: nothing of it is in the generated module), and an application without
# --gui has none of it. (The build of it needs the network - dmf-get fetches the converters - and is not tried here.)
set -euo pipefail
CHECKOUT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
fail() { echo "FAIL: $*"; exit 1; }

bash "$CHECKOUT/scripts/new-module.sh" --name guiapp --type application --path "$WORK/gui" --gui --skip-claude-sync > "$WORK/gui.log" 2>&1 || { cat "$WORK/gui.log"; fail "--gui"; }
bash "$CHECKOUT/scripts/new-module.sh" --name plainapp --type application --path "$WORK/plain" --skip-claude-sync > "$WORK/plain.log" 2>&1 || { cat "$WORK/plain.log"; fail "without --gui"; }

for f in views/guiapp.html views/guiapp.html.ini views/dmod.js \
         icons/icon.svg icons/icon.svg.ini docs/gui.md src/guiapp.c guiapp.dmr CMakeLists.txt tests/CMakeLists.txt; do
    [[ -s "$WORK/gui/$f" ]] || fail "$f is missing"
done
[[ ! -e "$WORK/gui/views/fonts" ]] || fail "the module has a font of its own (dmview has its fonts)"

# Nothing of the templates is left (the Makefile's @DMOD_DIR@ is for --dmod-dir: not this test's)
if grep -rnE "@(MODULE_NAME|MODULE_NAME_UPPER|AUTHOR_NAME|LICENSE|GUI_[A-Z_]*|CI_BADGE_BLOCK|PORT_[A-Z_]*)@" "$WORK/gui" -I | grep -q .; then
    grep -rnE "@(MODULE_NAME|MODULE_NAME_UPPER|AUTHOR_NAME|LICENSE|GUI_[A-Z_]*|CI_BADGE_BLOCK|PORT_[A-Z_]*)@" "$WORK/gui" -I | head
    fail "a placeholder is left"
fi
if grep -rn "@GUI_\|views/\|libdmview" "$WORK/plain/CMakeLists.txt" "$WORK/plain/tests/CMakeLists.txt" "$WORK/plain/src" "$WORK/plain/plainapp.dmr" | grep -q .; then
    fail "an application without --gui has the interface in it"
fi

# The wiring: the pages and the icon of the module, the libraries of the claim; the test is not an application
grep -q "DMOD_ASSETS_PATHS *\${CMAKE_CURRENT_SOURCE_DIR}/views" "$WORK/gui/CMakeLists.txt" || fail "DMOD_ASSETS_PATHS"
grep -q "DMOD_ICON_PATH *\${CMAKE_CURRENT_SOURCE_DIR}/icons" "$WORK/gui/CMakeLists.txt" || fail "DMOD_ICON_PATH"
grep -q "libdmview@>=" "$WORK/gui/CMakeLists.txt" || fail "libdmview"
grep -q 'DMOD_ASSETS_PATHS *""' "$WORK/gui/tests/CMakeLists.txt" || fail "the test has the view"
grep -q "views=./views => \${views_dir}" "$WORK/gui/guiapp.dmr" || fail "the .dmr has no views"
grep -q "icon=./icon.dmvi => \${icons_dir}/\${module}.dmvi" "$WORK/gui/guiapp.dmr" || fail "the .dmr has no icon"
grep -q 'libdmview_claim' "$WORK/gui/src/guiapp.c" || fail "main() does not claim a display"
grep -q "Graphical interface" "$WORK/gui/README.md" || fail "the README has no section of it"

# A library has no interface: the option is ignored with a warning
bash "$CHECKOUT/scripts/new-module.sh" --name guilib --type library --path "$WORK/lib" --gui --skip-claude-sync > "$WORK/lib.log" 2>&1 || fail "library with --gui"
grep -qi "only supported for application" "$WORK/lib.log" || fail "no warning for a library"
[[ ! -e "$WORK/lib/views" ]] || fail "a library has views"
echo "OK"
