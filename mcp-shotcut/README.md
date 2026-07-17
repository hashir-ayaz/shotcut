# mcp-shotcut

TypeScript MCP server for the Shotcut take-home **EditorService** control plane.

## Prerequisites

1. Build and launch Shotcut locally (`./run.sh` from repo root).
2. Confirm the log contains: `ControlServer listening on ws://127.0.0.1:3847`

### Whisper (optional — auto captions)

Live speech-to-text uses Shotcut's Whisper integration (`whisper-cli` + model path in Settings). On this machine `whisper-cli` is **not** installed; use `shotcut_import_captions` with an SRT file, or install Whisper and configure Shotcut Settings before calling `shotcut_transcribe_captions`.

## Install

```bash
cd mcp-shotcut
npm install
```

## Run manually

```bash
SHOTCUT_WS_URL=ws://127.0.0.1:3847 npx tsx src/index.ts
```

## Cursor MCP config

Project config lives at `.cursor/mcp.json`:

```json
{
  "mcpServers": {
    "shotcut": {
      "command": "npx",
      "args": ["tsx", "${workspaceFolder}/mcp-shotcut/src/index.ts"],
      "env": {
        "SHOTCUT_WS_URL": "ws://127.0.0.1:3847"
      }
    }
  }
}
```

Restart the MCP server in Cursor after pulling Phase 2 changes so new tools appear.

## Tools

| Tool | RPC method |
|------|------------|
| `shotcut_ping` | `ping` |
| `shotcut_get_state` | `get_state` |
| `shotcut_open_project` | `open_project` |
| `shotcut_save_project` | `save_project` |
| `shotcut_add_clip` | `add_clip` |
| `shotcut_list_transition_presets` | `list_transition_presets` |
| `shotcut_apply_transition` | `apply_transition` |
| `shotcut_set_transition_duration` | `set_transition_duration` |
| `shotcut_get_transition` | `get_transition` |
| `shotcut_remove_transition` | `remove_transition` |
| `shotcut_list_animatable_properties` | `list_animatable_properties` |
| `shotcut_add_keyframe` | `add_keyframe` |
| `shotcut_remove_keyframe` | `remove_keyframe` |
| `shotcut_list_keyframes` | `list_keyframes` |
| `shotcut_set_clip_property` | `set_clip_property` |
| `shotcut_transcribe_captions` | `transcribe_captions` |
| `shotcut_get_job_status` | `get_job_status` |
| `shotcut_get_captions` | `get_captions` |
| `shotcut_set_caption_text` | `set_caption_text` |
| `shotcut_import_captions` | `import_captions` |

All business logic lives in Shotcut's C++ `EditorService`. This package is a thin MCP adapter.

## Explicit skips (CapCut take-home)

| CapCut ref | Feature | Status |
|------------|---------|--------|
| #2 | Speed curve | Out of scope |
| #4 | Clip animations | Stretch only |
| #6 | Denoise & stem separation | Out of scope |
