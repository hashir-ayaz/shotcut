# Local macOS setup (Homebrew)

This fork is built and run against **Homebrew** Qt 6 + MLT, not the official bundled Shotcut SDK.

| Item | Value |
|------|--------|
| Clone | `~/development/shotcut` |
| Fork | https://github.com/hashir-ayaz/shotcut |
| Upstream | `upstream` → https://github.com/mltframework/shotcut.git |
| Install prefix | `~/development/shotcut/install` |
| App | `install/Shotcut.app` |

## Dependencies

```bash
brew install cmake ninja pkg-config qt mlt ffmpeg fftw frei0r sdl2
```

## Configure (once, or after cleaning `build/`)

```bash
cd ~/development/shotcut
cmake -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt;/opt/homebrew" \
  -DCMAKE_INSTALL_PREFIX="$HOME/development/shotcut/install" \
  -S . -B build
```

## Build + install

```bash
cmake --build build -j"$(sysctl -n hw.ncpu)"
cmake --install build
```

**Always run `./run.sh` at least once after `cmake --install`.** Install overwrites the app binary; `run.sh` re-applies the launcher wrapper and bundles `libCuteLogger`.

## Run

```bash
./run.sh
# or
open install/Shotcut.app
```

Logs: `~/Library/Application Support/Meltytech/Shotcut/shotcut-log.txt`

## Critical Homebrew MLT path quirk

Shotcut/MLT defaults look for:

- `/opt/homebrew/lib/mlt-7` (plugins)
- `/opt/homebrew/share/mlt-7` (profiles / data)

Homebrew actually installs:

- `$(brew --prefix mlt)/lib/mlt`
- `$(brew --prefix mlt)/share/mlt`

If those env vars are wrong, you get:

- `mlt_repository_init: no plugins found in "/opt/homebrew/lib/mlt-7"`
- **Failed to open** media (e.g. MP4)
- **There was an error saving** when creating a project

`run.sh` (and the wrapper it installs as `Shotcut.app/Contents/MacOS/Shotcut`) sets:

```bash
MLT_REPOSITORY="$(brew --prefix mlt)/lib/mlt"
MLT_DATA="$(brew --prefix mlt)/share/mlt"
MLT_PROFILES_PATH="$(brew --prefix mlt)/share/mlt/profiles"
FREI0R_PATH="$(brew --prefix frei0r)/lib/frei0r-1"
```

It also:

1. Copies `install/lib/libCuteLogger.dylib` into `Shotcut.app/Contents/Frameworks`
2. Moves the real binary to `Shotcut.bin` and wraps `Shotcut` so `open Shotcut.app` gets the same env

## Rebuild checklist

1. `cmake --build build -j"$(sysctl -n hw.ncpu)"`
2. `cmake --install build`
3. `./run.sh` (rewrap + launch)
4. If media/save fails again, check the log for `no plugins found` / `Failed to open properties file`

## Sync with upstream

```bash
git fetch upstream
git merge upstream/master   # or rebase
```

## MCP control plane (Phase 1)

Shotcut exposes an in-process **EditorService** over a localhost JSON-RPC WebSocket. The TypeScript MCP package in `mcp-shotcut/` is a thin adapter for Cursor.

| Item | Value |
|------|--------|
| WebSocket | `ws://127.0.0.1:3847` (override with `SHOTCUT_MCP_PORT`) |
| MCP package | `mcp-shotcut/` |
| Cursor config | `.cursor/mcp.json` |

### Start Shotcut + MCP

```bash
# terminal 1
./run.sh

# terminal 2 (optional manual MCP process)
cd mcp-shotcut && npm install && npx tsx src/index.ts
```

Confirm in the app log:

```bash
rg "ControlServer listening" ~/Library/Application\ Support/Meltytech/Shotcut/shotcut-log.txt | tail -1
```

### Manual RPC smoke

```bash
node --input-type=module <<'EOF'
import WebSocket from './mcp-shotcut/node_modules/ws/wrapper.mjs';
const ws = new WebSocket('ws://127.0.0.1:3847');
ws.on('open', () => ws.send(JSON.stringify({jsonrpc:'2.0',id:1,method:'ping',params:{}})));
ws.on('message', (d) => { console.log(d.toString()); ws.close(); });
EOF
```

### Cursor tools

- `shotcut_ping`
- `shotcut_get_state`
- `shotcut_open_project` `{ path }`
- `shotcut_save_project` `{ path? }`
- `shotcut_add_clip` `{ path, trackIndex?, position? }`

See `mcp-shotcut/README.md` for full MCP setup.
