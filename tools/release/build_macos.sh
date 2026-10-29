#!/bin/bash
# Builds the macOS release of the whole suite from scratch (universal:
# Apple silicon and Intel, macOS 10.15+):
#   build/release/AudbolPluginSuite-<version>-macos-vst3.zip
#   build/release/SHA256SUMS-macos.txt
#
#   tools/release/build_macos.sh
#
# Needs Xcode (or its command line tools) and CMake 3.25+. The bundles are
# ad-hoc signed, not notarized: see README-macos.txt for opening them.
# The GitHub Actions workflow (.github/workflows/build.yml) runs this.
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
out="$root/build/release"
build="$root/build-release-macos"

version=$(sed -nE 's/^[[:space:]]*VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$root/CMakeLists.txt" | head -1)
echo "Audbol Plugin Suite $version (macOS)"

# Clean build; the SDK validator runs on every plug-in.
rm -rf "$build"
cmake -S "$root" -B "$build" -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DSMTG_BUILD_UNIVERSAL_BINARY=ON > /dev/null
log="$build/build.log"
if ! cmake --build "$build" --config Release > "$log" 2>&1; then
    grep -E "error|FAILED" "$log" | head -40
    echo "Build failed" >&2
    exit 1
fi
passed=$(grep -c "tests passed, 0 tests failed" "$log" || true)
if [ "$passed" -lt 14 ] || grep -qE "tests passed, [1-9][0-9]* tests failed" "$log"; then
    grep -E "Result:" "$log"
    echo "Validation failed ($passed plug-ins passed)" >&2
    exit 1
fi
echo "built and validated $passed plug-ins"

stage="$build/stage"
mkdir -p "$stage"
count=0
for p in "$build"/VST3/Release/Audbol*.vst3; do
    name=$(basename "$p" .vst3)
    archs=$(lipo -archs "$p/Contents/MacOS/$name")
    [[ "$archs" == *x86_64* && "$archs" == *arm64* ]] || { echo "$name is not universal: $archs" >&2; exit 1; }
    ditto "$p" "$stage/$name.vst3"
    codesign --force --sign - --timestamp=none "$stage/$name.vst3"
    count=$((count + 1))
done
[ "$count" -eq 14 ] || { echo "expected 14 plug-ins, found $count" >&2; exit 1; }

"$build/bin/Release/make_presets" "$build/presets/Audbol Audio" | tail -1
mkdir -p "$stage/Presets"
ditto "$build/presets/Audbol Audio" "$stage/Presets/Audbol Audio"
cp "$root/tools/release/README-macos.txt" "$stage/README.txt"

mkdir -p "$out"
zip="AudbolPluginSuite-$version-macos-vst3.zip"
rm -f "$out/$zip"
ditto -c -k "$stage" "$out/$zip"
(cd "$out" && shasum -a 256 "$zip" > SHA256SUMS-macos.txt)
(cd "$out" && stat -f "%N %z" "$zip" | awk '{printf "%-48s %8.0f KB\n", $1, $2 / 1024}')
