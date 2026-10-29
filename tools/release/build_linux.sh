#!/bin/bash
# Builds the Linux release of the whole suite from scratch:
#   build/release/AudbolPluginSuite-<version>-linux-x86_64-vst3.zip
#   build/release/SHA256SUMS-linux.txt
#
# On Linux:     tools/release/build_linux.sh
# From Windows: wsl -d Ubuntu-22.04 --exec bash tools/release/build_linux.sh
#
# A checkout on a Windows drive (/mnt/...) is copied into ~/audbol-src first:
# building across the WSL file-system bridge is many times slower.
#
# Needs CMake 3.25+, GCC or Clang, Ninja, zip and the VSTGUI dependencies:
#   sudo apt install build-essential ninja-build zip libx11-xcb-dev libxcb-util-dev \
#     libxcb-cursor-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev \
#     libfontconfig1-dev libcairo2-dev libgtkmm-3.0-dev libsqlite3-dev libxcb-keysyms1-dev
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
out="$root/build/release"
src="$root"
if [[ "$root" == /mnt/* ]]; then
    src="$HOME/audbol-src"
    mkdir -p "$src"
    rsync -a --delete --exclude '/build*' --exclude '/FanControl' --exclude '.git' \
        --exclude '/tools/reaper/signals' "$root/" "$src/"
fi
build="$src/build-release"

version=$(grep -oP '^\s*VERSION\s+\K[0-9]+\.[0-9]+\.[0-9]+' "$src/CMakeLists.txt" | head -1)
echo "Audbol Plugin Suite $version (Linux)"

# Clean build; the SDK validator runs on every plug-in.
rm -rf "$build"
cmake -S "$src" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release > /dev/null
log="$build/build.log"
if ! cmake --build "$build" > "$log" 2>&1; then
    grep -E "error|FAILED" "$log" | head -20
    echo "Build failed" >&2
    exit 1
fi
passed=$(grep -c "tests passed, 0 tests failed" "$log" || true)
if [ "$passed" -lt 14 ] || grep -qE "tests passed, [1-9][0-9]* tests failed" "$log"; then
    echo "Validation failed ($passed plug-ins passed)" >&2
    exit 1
fi
echo "built and validated $passed plug-ins"

stage="$build/stage"
mkdir -p "$stage"
count=0
for p in "$build"/VST3/Release/Audbol*.vst3; do
    cp -r "$p" "$stage/"
    count=$((count + 1))
done
[ "$count" -eq 14 ] || { echo "expected 14 plug-ins, found $count" >&2; exit 1; }
# Symbols aren't shipped (the validator has already run on the full binaries).
strip --strip-unneeded "$stage"/*.vst3/Contents/x86_64-linux/*.so

"$build/bin/Release/make_presets" "$build/presets/Audbol Audio" | tail -1
mkdir -p "$stage/Presets"
cp -r "$build/presets/Audbol Audio" "$stage/Presets/"
cp "$root/tools/release/README-linux.txt" "$stage/README.txt"

mkdir -p "$out"
zip="AudbolPluginSuite-$version-linux-x86_64-vst3.zip"
rm -f "$out/$zip"
(cd "$stage" && zip -qr "$out/$zip" .)
(cd "$out" && sha256sum "$zip" > SHA256SUMS-linux.txt)
(cd "$out" && stat -c "%n %s" "$zip" | awk '{printf "%-48s %8.0f KB\n", $1, $2 / 1024}')
