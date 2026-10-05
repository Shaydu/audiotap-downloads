#!/bin/sh
# Build the Stereo Gain example, sign it (macOS, ad-hoc) and validate it.
# Usage: ./build.sh            result: build/StereoGain.clap
# Needs: cmake, a C compiler, git (to fetch the CLAP headers). Optional: clap-validator on PATH
# (or set CLAP_VALIDATOR=/path/to/clap-validator).
set -eu
cd "$(dirname "$0")"

if [ "$(uname)" = "Darwin" ]; then
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
else
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
fi
cmake --build build --config Release

if [ "$(uname)" = "Darwin" ]; then
  PLUGIN=build/StereoGain.clap
  [ -d "$PLUGIN" ] || PLUGIN=build/Release/StereoGain.clap
  # Ad-hoc signature: AudioTap loads this only with "Allow plugins I built myself" on.
  codesign -s - --force --deep "$PLUGIN"
  codesign --verify --deep --strict "$PLUGIN"
  lipo -archs "$PLUGIN/Contents/MacOS/StereoGain"
else
  PLUGIN=build/StereoGain.clap
fi
echo "Built $PLUGIN"

VALIDATOR="${CLAP_VALIDATOR:-$(command -v clap-validator || true)}"
if [ -n "$VALIDATOR" ]; then
  "$VALIDATOR" validate "$PLUGIN"
else
  echo "clap-validator not found; skipping validation."
  echo "Get it from https://github.com/free-audio/clap-validator/releases and run:"
  echo "  clap-validator validate $PLUGIN"
fi
