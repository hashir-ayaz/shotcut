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

## Live demo session — 2026-07-18 (MCP)

Two **barn-door** transitions between visually distinct sources, on a fresh `testdata/blank-1080.mlt` (1920×1080 @ 29.97 fps). Layout **bbb → ted → bbb** so each join sits between different footage.

1. `shotcut_ping` → `{ pong: true, version: "26.7.18" }`
2. `shotcut_get_state` → blank track 0 (single 1-frame blank).
3. `shotcut_list_transition_presets` → 13 presets incl. `wipe_barn_door_horizontal` (`%luma03.pgm`), `wipe_barn_door_vertical` (`%luma04.pgm`).
4. `shotcut_add_clip` `bbb-60s.mp4` `{ inPoint:0, outPoint:300, append:true }` → clip 0 (frames 0–300).
5. `shotcut_add_clip` `ted-talk-90s.mp4` `{ inPoint:90, outPoint:390, append:true }` → clip 1 (frames 301–601). `inPoint:90` gives ≥60f headroom for the trim-in.
6. `shotcut_add_clip` `bbb-60s.mp4` `{ inPoint:90, outPoint:390, append:true }` → clip 2 (frames 602–902).
7. **Join 1** — `shotcut_apply_transition` `{ trackIndex:0, clipIndex:1, presetId:"wipe_barn_door_horizontal", durationFrames:60 }` → ok, `transitionClipIndex:1`.
8. `shotcut_get_state` (indices shifted) → `0` bbb (0–240), **`1` transition H (241–300)**, `2` ted (301–601), `3` bbb (602–902).
9. **Join 2** — `shotcut_apply_transition` `{ trackIndex:0, clipIndex:3, presetId:"wipe_barn_door_vertical", durationFrames:60 }` → ok, `transitionClipIndex:3`.
10. `shotcut_get_state` (final):

| Index | Clip | Frames | isTransition | Preset |
|---|---|---|---|---|
| 0 | bbb-60s.mp4 | 0–240 | false | — |
| 1 | `<tractor>` | 241–300 | true | `wipe_barn_door_horizontal` (`%luma03.pgm`) |
| 2 | ted-talk-90s.mp4 | 301–541 | false | — |
| 3 | `<tractor>` | 542–601 | true | `wipe_barn_door_vertical` (`%luma04.pgm`) |
| 4 | bbb-60s.mp4 | 602–902 | false | — |

**Confirmed:** both transitions materialize as their own `isTransition:true` clips; indices shift on each apply (re-read `get_state` before the next op). Scrub **241–300** (horizontal split) and **542–601** (vertical split).
