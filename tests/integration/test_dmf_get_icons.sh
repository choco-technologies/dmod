#!/bin/bash
# Icons: the "icon" resource of a .dmr, [screen=ID] for an icon of its own for a screen, [variant=NAME] for another one (a size) in a
# subdirectory of the icons directory. dmf-get installs the default icon and the variants, with the screen's own where there is one.
set -euo pipefail
BUILD_DIR="$(cd "${1:-../../build}" && pwd)"
DMF_GET="$BUILD_DIR/bin/tools/dmf-get"
MKDMRPKG="$BUILD_DIR/bin/tools/mkdmrpkg"
TEST_DIR="$(mktemp -d)"
trap 'rm -rf "$TEST_DIR"' EXIT
cd "$TEST_DIR"
export DMOD_CACHE_DIR="$TEST_DIR/cache"
unset DMOD_SCREEN DMOD_PACKAGE_DIR DMOD_VIEWS_DIR DMOD_LICENSE_DIR DMOD_ICONS_DIR
fail() { echo "FAIL: $*"; cat install.log; exit 1; }

# The icons as the build leaves them: <icons>[/<variant>][/<screen>]/<module>.dmvi
mkdir -p source icons/small/color icons/epaper
printf 'binary' > source/app.dmf
: > source/app.dmd
printf 'default' > icons/app.dmvi
printf 'epaper' > icons/epaper/app.dmvi
printf 'small' > icons/small/app.dmvi
printf 'small color' > icons/small/color/app.dmvi
mkdir -p source/views/a source/views/b
printf 'a' > source/views/a/main.dmv
printf 'b' > source/views/b/main.dmv
cat > source/app.dmr <<'DMR'
dmf=./app.dmf => ${destination}/app.dmf [origin=${repo_dir}/app.dmf]
dmd=./app.dmd => ${destination}/app.dmd [origin=${repo_dir}/app.dmd]
dmr=./app.dmr => ${destination}/app/app.dmr [origin=${repo_dir}/app.dmr]
icon=./icon.dmvi => ${icons_dir}/${module}.dmvi [origin=${icons_dir}/${module}.dmvi]
icon=./icon-epaper.dmvi => ${icons_dir}/${module}.dmvi [screen=epaper] [origin=${icons_dir}/epaper/${module}.dmvi]
icon=./icon-small.dmvi => ${icons_dir}/${module}.dmvi [variant=small] [origin=${icons_dir}/${module}.dmvi]
icon=./icon-small-color.dmvi => ${icons_dir}/${module}.dmvi [variant=small] [screen=color] [origin=${icons_dir}/color/${module}.dmvi]
views=./views/a => ${views_dir} [screen=epaper] [default=true] [origin=${repo_dir}/views/a]
views=./views/b => ${views_dir} [screen=color] [origin=${repo_dir}/views/b]
DMR
"$MKDMRPKG" source/app.dmr -m app -r "$TEST_DIR/source" --icons-dir "$TEST_DIR/icons" -o package > install.log 2>&1 || fail package
cmp icons/app.dmvi package/icon.dmvi
cmp icons/epaper/app.dmvi package/icon-epaper.dmvi
cmp icons/small/app.dmvi package/icon-small.dmvi                # (the variant is a directory of ${icons_dir}: the origin is found)
cmp icons/small/color/app.dmvi package/icon-small-color.dmvi
# Without --icons-dir the icons are left out of the package (a project without icons), the rest is packed
"$MKDMRPKG" source/app.dmr -m app -r "$TEST_DIR/source" -o package-no-icons > install.log 2>&1 || fail "package without icons"
[ ! -e package-no-icons/icon.dmvi ] || fail "an icon without --icons-dir"
test -f package-no-icons/app.dmf
(cd package && zip -qr ../app.zip .)
echo "app $TEST_DIR/app.zip" > manifest.dmm
args=(-m "$TEST_DIR/manifest.dmm" --no-fallback --skip-arch-check --skip-dmod-ver-check -y)
run() { "$DMF_GET" "${args[@]}" "$@" > install.log 2>&1 || fail "$*"; }
reject() { if "$DMF_GET" "${args[@]}" "$@" > install.log 2>&1; then fail "expected failure: $*"; fi; }
files() { (cd "$1" && find . -type f -name '*.dmvi' | sort | tr '\n' ' '); }

# The screen epaper: its own icon, the default of the variant small
run app -o out --icons-dir "$TEST_DIR/inst" --screen epaper
[ "$(files inst)" = "./app.dmvi ./small/app.dmvi " ] || fail "epaper: $(files inst)"
cmp inst/app.dmvi icons/epaper/app.dmvi
cmp inst/small/app.dmvi icons/small/app.dmvi
# The screen color: the default icon, its own small one
run app -o out2 --icons-dir "$TEST_DIR/inst2" --screen color
cmp inst2/app.dmvi icons/app.dmvi
cmp inst2/small/app.dmvi icons/small/color/app.dmvi
# No screen: the defaults
run app -o out3 --icons-dir "$TEST_DIR/inst3" --screen epaper --mini
[ "$(files inst3)" = "./app.dmvi ./small/app.dmvi " ] || fail "mini: $(files inst3)"        # (--mini keeps the icons)
run app -o out4 --mini --views-dir "$TEST_DIR/views4" --screen color
[ "$(files out4)" = "./icons/app.dmvi ./icons/small/app.dmvi " ] || fail "default directory: $(files out4)"     # <output>/icons
cmp views4/app/main.dmv source/views/b/main.dmv                  # (the views of the screen are still chosen by it)
DMOD_ICONS_DIR="$TEST_DIR/env-icons" run app -o out5 --screen color
cmp env-icons/app.dmvi icons/app.dmvi

# The icon command: just the icons (the package is not installed)
run icon app -o "$TEST_DIR/only" --screen epaper
[ "$(files only)" = "./app.dmvi ./small/app.dmvi " ] || fail "icon command: $(files only)"
cmp only/app.dmvi icons/epaper/app.dmvi
[ ! -e only/app.dmf ] || fail "the icon command installed the module"
DMOD_ICONS_DIR="$TEST_DIR/only-env" run icon app
[ "$(files only-env)" = "./app.dmvi ./small/app.dmvi " ] || fail "icon command, DMOD_ICONS_DIR: $(files only-env)"

# A module with no icon: the icon command fails, an install is fine
mkdir -p source-plain
printf 'binary' > source-plain/plain.dmf
: > source-plain/plain.dmd
cat > source-plain/plain.dmr <<'DMR'
dmf=./plain.dmf => ${destination}/plain.dmf [origin=${repo_dir}/plain.dmf]
dmd=./plain.dmd => ${destination}/plain.dmd [origin=${repo_dir}/plain.dmd]
dmr=./plain.dmr => ${destination}/plain/plain.dmr [origin=${repo_dir}/plain.dmr]
DMR
"$MKDMRPKG" source-plain/plain.dmr -m plain -r "$TEST_DIR/source-plain" -o package-plain > install.log 2>&1 || fail "package plain"
(cd package-plain && zip -qr ../plain.zip .)
echo "plain $TEST_DIR/plain.zip" >> manifest.dmm
reject icon plain -o "$TEST_DIR/none"
run plain -o out6 --icons-dir "$TEST_DIR/inst6"
[ ! -e inst6 ] || [ -z "$(files inst6)" ] || fail "icons of a module that has none"

# Wrong resource files are refused while parsing: an icon variant has an identifier, and only icons have one
cat > source-plain/bad.dmr <<'DMR'
dmf=./plain.dmf => ${destination}/plain.dmf [variant=small]
DMR
if "$MKDMRPKG" source-plain/bad.dmr -m plain -r "$TEST_DIR/source-plain" -o package-bad > install.log 2>&1; then fail "a variant on a module file"; fi
echo "dmf-get icons: OK"
