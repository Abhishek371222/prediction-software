#!/bin/zsh
# Build Atomik .app + DMG for sharing (Apple Silicon, macOS 15+).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

VERSION="1.3.0"
APP_NAME="Atomik Acoustic Simulation Engine"
DMG_NAME="Atomik_Acoustic_Simulation_Engine_v${VERSION}_macOS"
OUT_DIR="$ROOT/dist/mac"
STAGE="$OUT_DIR/_dmg_stage"
APP="$STAGE/${APP_NAME}.app"
BIN_SRC="$ROOT/Builds/MacManual/build/TwoSpeakerExplorer"
ICNS_SRC="$ROOT/dist/mac/Atomik Acoustic Simulation Engine.app/Contents/Resources/Atomik.icns"

echo "=== 1) Rebuild binary ==="
zsh "$ROOT/Builds/MacManual/build_macos15.sh"

if [[ ! -x "$BIN_SRC" ]]; then
  echo "Missing binary: $BIN_SRC" >&2
  exit 1
fi

echo "=== 2) Stage clean DMG root ==="
rm -rf "$STAGE"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"

# Info.plist
cat > "$APP/Contents/Info.plist" <<EOF
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
</dict>
</plist>
EOF
echo -n "APPL????" > "$APP/Contents/PkgInfo"

# Binary + icon
cp "$BIN_SRC" "$APP/Contents/MacOS/TwoSpeakerExplorer"
chmod +x "$APP/Contents/MacOS/TwoSpeakerExplorer"
if [[ -f "$ICNS_SRC" ]]; then
  cp "$ICNS_SRC" "$APP/Contents/Resources/Atomik.icns"
fi

echo "=== 3) Bundle Assets + measurements + docs ==="
# Brand assets (fonts, logos — including ATOMIK-only crops)
rsync -a --delete \
  --exclude '.DS_Store' \
  "$ROOT/Assets/" "$APP/Contents/Resources/Assets/"

# Measurement CSV pack (primary runtime data)
mkdir -p "$APP/Contents/Resources/prediction software"
rsync -a --delete \
  --exclude '.DS_Store' \
  "$ROOT/prediction software/MeasurementIntegrationPack/" \
  "$APP/Contents/Resources/prediction software/MeasurementIntegrationPack/"

# Guild measurements (.xlsx) + export.md / analysis helpers
rsync -a --delete \
  --exclude '.DS_Store' \
  --exclude '__pycache__' \
  "$ROOT/shyamGuildMeasurements/" \
  "$APP/Contents/Resources/shyamGuildMeasurements/"

# Documentation folder next to the app (easy to open / share)
DOCS="$STAGE/Documentation"
mkdir -p "$DOCS"
cp "$ROOT/shyamGuildMeasurements/export.md" "$DOCS/Export_Reference.md"
cp "$ROOT/Installer/README.txt" "$DOCS/Installer_README.txt" 2>/dev/null || true
cp "$ROOT/prediction software/MeasurementIntegrationPack/README.md" \
   "$DOCS/MeasurementIntegrationPack_README.md" 2>/dev/null || true
# Format example for Atomik directivity CSV (VACS-style header + Level/Phase)
if [[ -f "$ROOT/Source/Atomik_Directivity_50Hz_1p0m(PredictionSoftware).csv" ]]; then
  cp "$ROOT/Source/Atomik_Directivity_50Hz_1p0m(PredictionSoftware).csv" \
     "$DOCS/Sample_Directivity_Format_Example.csv"
fi

# Sample CSVs so recipients see the export / measurement format
mkdir -p "$DOCS/Sample_CSV"
cp "$ROOT/prediction software/MeasurementIntegrationPack/Data/"ShyamGuild_*.csv \
   "$DOCS/Sample_CSV/" 2>/dev/null || true
cp "$ROOT/prediction software/MeasurementIntegrationPack/Data/manifest.csv" \
   "$DOCS/Sample_CSV/" 2>/dev/null || true

cat > "$DOCS/README.txt" <<'EOF'
Atomik Acoustic Simulation Engine — Documentation
=================================================

Export_Reference.md
  What the app can export (PNG, SPL CSV, Directivity CSV, SVG, PDF),
  where each action lives, and the file formats.

Sample_CSV/
  Example measurement CSVs (ShyamGuild) and manifest.csv from the
  MeasurementIntegrationPack shipped inside the app.

MeasurementIntegrationPack_README.md
  How the bundled measurement pack is organised.

Inside the .app (Contents/Resources/):
  Assets/                  Brand fonts + logos
  prediction software/     MeasurementIntegrationPack (CSV data)
  shyamGuildMeasurements/  Room measurement workbooks + export.md

Exports from the running app (PNG / CSV / SVG / PDF) are chosen via
Export in the header, or ⋯ More → Export PDF Report. See Export_Reference.md.
EOF

cat > "$STAGE/READ ME - First Open.txt" <<EOF
${APP_NAME} v${VERSION}

Requires: macOS 15.0 or later (Apple Silicon — M1 / M2 / M3 / M4)

Install
-------
1. Drag "${APP_NAME}.app" into Applications.
2. First launch: right-click the app → Open → Open
   (needed once for unsigned builds; Gatekeeper may block a normal double-click).

Documentation
-------------
Open the Documentation folder for Export_Reference.md (PNG / CSV / SVG / PDF),
sample CSVs, and measurement notes.

(c) Atomik
EOF

# Applications drop-target symlink (standard DMG UX)
ln -sf /Applications "$STAGE/Applications"

echo "=== 4) Ad-hoc sign (local Gatekeeper friendliness) ==="
codesign --force --deep --sign - "$APP" 2>/dev/null || true

echo "=== 5) Create DMG ==="
mkdir -p "$OUT_DIR"
DMG_PATH="$HOME/Desktop/${DMG_NAME}.dmg"
rm -f "$DMG_PATH"
# Remove older/duplicate build outputs so only one shareable DMG exists
rm -f "$OUT_DIR/${DMG_NAME}.dmg"
setopt NULL_GLOB
rm -f "$HOME/Desktop"/Atomik_Acoustic_Simulation_Engine_v*_macOS.dmg
rm -f "$OUT_DIR"/Atomik_Acoustic_Simulation_Engine_v*_macOS.dmg
rm -f "$ROOT/dist/Atomik_Acoustic_Simulation_Engine_macOS.dmg"
unsetopt NULL_GLOB
# Recreate path after sweeping old Desktop DMGs
DMG_PATH="$HOME/Desktop/${DMG_NAME}.dmg"

# Refresh the standalone .app copy used for local testing
rm -rf "$OUT_DIR/${APP_NAME}.app"
cp -R "$APP" "$OUT_DIR/${APP_NAME}.app"

hdiutil create \
  -volname "Atomik v${VERSION}" \
  -srcfolder "$STAGE" \
  -ov -format UDZO \
  "$DMG_PATH"

echo
echo "=== done ==="
ls -lh "$DMG_PATH"
echo
echo "DMG (Desktop only): $DMG_PATH"
echo "Contains: ${APP_NAME}.app + Documentation/ (export.md, sample CSV, READMEs)"
