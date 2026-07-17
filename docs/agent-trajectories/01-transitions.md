# Agent Trajectory — Transitions (Phase 2.1)

## Goal

Apply CapCut-style dissolve/wipe transitions between two timeline clips via MCP, confirm readback in `get_state`, and persist in `.mlt`.

## Preconditions

- Shotcut built and running: `cmake --build build && cmake --install build && ./run.sh`
- ControlServer on `ws://127.0.0.1:3847`
- Test media: `testdata/mcp-test.mlt` + `/Users/hashirayaz/Documents/duplicate-test.mp4`

## Steps (agent-driven)

1. **Ping** — `shotcut_ping` → `{ ok: true, data: { pong: true } }`
2. **List presets** — `shotcut_list_transition_presets` → 13 presets (`dissolve`, `wipe_clock_top`, …)
3. **Open project** — `shotcut_open_project` with `testdata/mcp-test.mlt`
4. **Add second clip** — `shotcut_add_clip` with `duplicate-test.mp4` on track 0
5. **Apply dissolve** — `shotcut_apply_transition`:
   - `trackIndex: 0`, `clipIndex: 1` (incoming clip at join)
   - `presetId: "dissolve"`, `durationFrames: 24`
6. **Verify state** — `shotcut_get_state` shows transition clip:
   - `isTransition: true`, `transitionPreset: "dissolve"`, `duration: 24`
7. **Read transition** — `shotcut_get_transition` with `transitionClipIndex: 1` (use `index` field from state)
8. **Save** — `shotcut_save_project` → `testdata/phase2-verify.mlt`

## UI path (manual)

View → Transitions dock (Ctrl+Shift+8). Select incoming clip at join, pick preset, set duration, **Apply to Selected Join**.

## Result

Dissolve transition visible on timeline between clips; MCP and UI both route through `EditorService::applyTransition` → `Timeline::AddTransitionByTrimInCommand` + luma preset.

## Recording

Screen capture of UI apply + MCP `get_state` readback: `docs/recordings/01-transitions.mp4` (add locally).
