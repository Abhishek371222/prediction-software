#!/bin/zsh
# Build (if needed) and launch Atomik Integral as a real .app so it stays in Dock / Cmd+Tab.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APP="$ROOT/Builds/MacManual/build/Atomik Integral.app"
BIN="$ROOT/Builds/MacManual/build/TwoSpeakerExplorer"

if [[ ! -x "$BIN" || ! -d "$APP" ]]; then
  zsh "$ROOT/Builds/MacManual/build_macos15.sh"
fi

# Prefer killing only this product's processes (bundle id / app name / binary).
pkill -f "Atomik Integral.app/Contents/MacOS/TwoSpeakerExplorer" 2>/dev/null || true
pkill -f "/Builds/MacManual/build/TwoSpeakerExplorer" 2>/dev/null || true
sleep 0.3

open "$APP"
echo "Launched: $APP"
echo "It should stay visible in the Dock and Cmd+Tab while running."
