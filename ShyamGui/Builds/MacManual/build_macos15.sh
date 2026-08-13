#!/bin/zsh
# Rebuild Atomik for macOS 15+ on Apple Silicon (M1/M2/M3/M4).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

BUILD_DIR="Builds/MacManual/build"
mkdir -p "$BUILD_DIR"

# Prefer the macOS 15 SDK so we don't accidentally link macOS 26-only APIs.
SDK=""
for candidate in \
  /Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk \
  /Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk \
  /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
do
  if [[ -d "$candidate" ]]; then
    SDK="$candidate"
    break
  fi
done

if [[ -z "$SDK" ]]; then
  echo "No macOS SDK found." >&2
  exit 1
fi

MIN_OS="15.0"
ARCH="arm64"

echo "SDK:    $SDK"
echo "min OS: $MIN_OS"
echo "arch:   $ARCH"

common=(
  -std=c++17
  -stdlib=libc++
  -O2
  -DNDEBUG=1
  -arch "$ARCH"
  -isysroot "$SDK"
  -mmacosx-version-min="$MIN_OS"
  -IBuilds/MacManual
  -ISource
  -Iexternal/JUCE/modules
  -include Builds/MacManual/AppConfig.h
  # Quiet third-party / toolchain noise (JUCE macOS 15 deprecations, START_JUCE_APPLICATION).
  -Wno-deprecated-declarations
  -Wno-main
  -Wno-unused-command-line-argument
)

frameworks=(
  -framework Cocoa
  -framework Foundation
  -framework AppKit
  -framework CoreFoundation
  -framework CoreServices
  -framework CoreGraphics
  -framework CoreText
  -framework ImageIO
  -framework QuartzCore
  -framework IOKit
  -framework Carbon
  -framework Accelerate
  -framework AudioToolbox
  -framework CoreAudio
  -framework CoreMIDI
  -framework Metal
  -framework MetalKit
  -framework CoreVideo
  -framework UniformTypeIdentifiers
  -framework Security
)

rm -f "$BUILD_DIR"/*.o(N) "$BUILD_DIR"/TwoSpeakerExplorer(N)

compile_one() {
  local src="$1"
  local out="$2"
  echo "  compile $(basename "$src")"
  clang++ "${common[@]}" -x objective-c++ -c "$src" -o "$out"
}

compile_one "external/JUCE/modules/juce_core/juce_core.cpp"                         "$BUILD_DIR/juce_core.o"
compile_one "external/JUCE/modules/juce_data_structures/juce_data_structures.cpp"   "$BUILD_DIR/juce_data_structures.o"
compile_one "external/JUCE/modules/juce_events/juce_events.cpp"                     "$BUILD_DIR/juce_events.o"
compile_one "external/JUCE/modules/juce_graphics/juce_graphics.cpp"                 "$BUILD_DIR/juce_graphics.o"
compile_one "external/JUCE/modules/juce_gui_basics/juce_gui_basics.cpp"             "$BUILD_DIR/juce_gui_basics.o"
compile_one "external/JUCE/modules/juce_gui_extra/juce_gui_extra.cpp"               "$BUILD_DIR/juce_gui_extra.o"

for src in \
  Source/Main.cpp \
  Source/MainComponent.cpp \
  Source/AcousticEngine.cpp \
  Source/RadiationPatternComponent.cpp \
  Source/ControlPanel.cpp \
  Source/InfoPanel.cpp \
  Source/PreferencesComponent.cpp \
  Source/DashboardComponent.cpp
do
  compile_one "$src" "$BUILD_DIR/$(basename "${src%.cpp}").o"
done

echo "  link TwoSpeakerExplorer"
clang++ "${common[@]}" \
  "$BUILD_DIR"/*.o \
  "${frameworks[@]}" \
  -o "$BUILD_DIR/TwoSpeakerExplorer"

# ---------------------------------------------------------------------------
# Assemble a real .app so macOS keeps it in the Dock / Cmd+Tab switcher.
# Launching the bare Mach-O with `open` makes the app vanish from running
# apps when you switch away — it looks "closed" even though the process lives.
# ---------------------------------------------------------------------------
APP_NAME="Atomik Acoustic Simulation Engine"
APP_DIR="$BUILD_DIR/${APP_NAME}.app"
VERSION="1.2.5"
ICNS_CANDIDATES=(
  "$ROOT/dist/mac/Atomik Acoustic Simulation Engine.app/Contents/Resources/Atomik.icns"
  "$ROOT/Assets/Atomik.icns"
)

echo "  assemble ${APP_NAME}.app"
mkdir -p "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources"
cp "$BUILD_DIR/TwoSpeakerExplorer" "$APP_DIR/Contents/MacOS/TwoSpeakerExplorer"
chmod +x "$APP_DIR/Contents/MacOS/TwoSpeakerExplorer"
echo -n "APPL????" > "$APP_DIR/Contents/PkgInfo"
cat > "$APP_DIR/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>en</string>
  <key>CFBundleExecutable</key>
  <string>TwoSpeakerExplorer</string>
  <key>CFBundleIconFile</key>
  <string>Atomik</string>
  <key>CFBundleIdentifier</key>
  <string>com.atomikaudio.acousticsimulationengine</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>${APP_NAME}</string>
  <key>CFBundleDisplayName</key>
  <string>${APP_NAME}</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>${VERSION}</string>
  <key>CFBundleVersion</key>
  <string>${VERSION}</string>
  <key>LSMinimumSystemVersion</key>
  <string>15.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
  <key>LSUIElement</key>
  <false/>
</dict>
</plist>
EOF
for icns in "${ICNS_CANDIDATES[@]}"; do
  if [[ -f "$icns" ]]; then
    cp "$icns" "$APP_DIR/Contents/Resources/Atomik.icns"
    break
  fi
done
# Refresh LaunchServices so Dock / Cmd+Tab pick up the new build.
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister \
  -f "$APP_DIR" 2>/dev/null || true

echo
echo "=== verify ==="
file "$BUILD_DIR/TwoSpeakerExplorer"
lipo -info "$BUILD_DIR/TwoSpeakerExplorer"
otool -l "$BUILD_DIR/TwoSpeakerExplorer" | awk '/LC_BUILD_VERSION/{p=1} p&&/minos|sdk|platform/{print; if(/sdk/) exit}'
echo "App: $APP_DIR"
echo "Build OK."
echo "Run with:  open \"$APP_DIR\""

# Optional: rebuild && run — pass --run (kills prior instance so the new build shows).
if [[ "${1:-}" == "--run" ]]; then
  pkill -x TwoSpeakerExplorer 2>/dev/null || true
  sleep 0.4
  rm -f "$HOME/Library/Caches/com.juce.locks/juceAppLock_Atomik Acoustic Simulation Engine"
  open "$APP_DIR"
  echo "Launched."
fi
