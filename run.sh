#!/usr/bin/env bash
# Launch the locally built Shotcut (Homebrew Qt/MLT).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
APP="$ROOT/install/Shotcut.app"
MACOS="$APP/Contents/MacOS"
BIN="$MACOS/Shotcut"
REAL="$MACOS/Shotcut.bin"

if [[ ! -e "$BIN" && ! -e "$REAL" ]]; then
  echo "Shotcut not built/installed yet."
  echo "  cmake --build \"$ROOT/build\" -j\"\$(sysctl -n hw.ncpu)\""
  echo "  cmake --install \"$ROOT/build\""
  echo "  \"$ROOT/run.sh\""
  exit 1
fi

MLT_ROOT="$(brew --prefix mlt)"
FREI0R_ROOT="$(brew --prefix frei0r)"

# Bundle CuteLogger (cmake install puts it in install/lib).
mkdir -p "$APP/Contents/Frameworks"
if [[ -f "$ROOT/install/lib/libCuteLogger.dylib" ]]; then
  cp -f "$ROOT/install/lib/libCuteLogger.dylib" "$APP/Contents/Frameworks/"
  install_name_tool -id "@rpath/libCuteLogger.dylib" \
    "$APP/Contents/Frameworks/libCuteLogger.dylib" 2>/dev/null || true
fi

# After cmake --install, Shotcut is a Mach-O binary again. Wrap it so
# `open Shotcut.app` also gets the correct Homebrew MLT paths.
# Homebrew uses lib/mlt + share/mlt; default MLT expects lib/mlt-7 + share/mlt-7.
if [[ -f "$BIN" ]] && file "$BIN" | grep -q 'Mach-O'; then
  mv "$BIN" "$REAL"
fi
if [[ -f "$REAL" ]]; then
  if ! otool -l "$REAL" | grep -q '@executable_path/../Frameworks'; then
    install_name_tool -add_rpath "@executable_path/../Frameworks" "$REAL" 2>/dev/null || true
  fi
  cat > "$BIN" << WRAP
#!/bin/bash
export PATH="/opt/homebrew/bin:\${PATH}"
export MLT_REPOSITORY="${MLT_ROOT}/lib/mlt"
export MLT_DATA="${MLT_ROOT}/share/mlt"
export MLT_PROFILES_PATH="${MLT_ROOT}/share/mlt/profiles"
export FREI0R_PATH="${FREI0R_ROOT}/lib/frei0r-1"
exec "\$(dirname "\$0")/Shotcut.bin" "\$@"
WRAP
  chmod +x "$BIN"
fi

export PATH="/opt/homebrew/bin:${PATH}"
export MLT_REPOSITORY="${MLT_REPOSITORY:-$MLT_ROOT/lib/mlt}"
export MLT_DATA="${MLT_DATA:-$MLT_ROOT/share/mlt}"
export MLT_PROFILES_PATH="${MLT_PROFILES_PATH:-$MLT_ROOT/share/mlt/profiles}"
export FREI0R_PATH="${FREI0R_PATH:-$FREI0R_ROOT/lib/frei0r-1}"

exec "$BIN" "$@"
