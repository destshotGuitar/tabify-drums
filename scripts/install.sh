#!/usr/bin/env bash
# Build Tabify Drums and install the VST3, CLAP and LV2 plugins for the
# current user (Linux). Run from anywhere: scripts/install.sh
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build"
out="$build/TabifyDrums_artefacts/Release"

cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --target TabifyDrums_VST3 TabifyDrums_CLAP TabifyDrums_LV2 TabifyDrums_Standalone

mkdir -p ~/.vst3 ~/.clap ~/.lv2
rm -rf ~/.vst3/"Tabify Drums.vst3" ~/.clap/"Tabify Drums.clap" ~/.lv2/"Tabify Drums.lv2"
cp -R "$out/VST3/Tabify Drums.vst3" ~/.vst3/
cp "$out/CLAP/Tabify Drums.clap" ~/.clap/
cp -R "$out/LV2/Tabify Drums.lv2" ~/.lv2/

echo
echo "Installed to ~/.vst3, ~/.clap and ~/.lv2. Rescan plugins in your DAW."
echo "Standalone app: $out/Standalone/Tabify Drums"
