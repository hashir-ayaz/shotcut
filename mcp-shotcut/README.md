# mcp-shotcut

TypeScript MCP server for the Shotcut take-home **EditorService** control plane.

## Prerequisites

1. Build and launch Shotcut locally (`./run.sh` from repo root).
2. Confirm the log contains: `ControlServer listening on ws://127.0.0.1:3847`

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

## Tools

| Tool | RPC method |
|------|------------|
| `shotcut_ping` | `ping` |
| `shotcut_get_state` | `get_state` |
| `shotcut_open_project` | `open_project` |
| `shotcut_save_project` | `save_project` |
| `shotcut_add_clip` | `add_clip` |

All business logic lives in Shotcut's C++ `EditorService`. This package is a thin MCP adapter.
