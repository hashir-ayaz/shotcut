# Agent Trajectory — Speed Curve (Phase 2.6)

## Goal

Apply CapCut-style speed curve presets via timeremap `speed_map`, inspect readback, apply constant/custom speeds, remove, and confirm persistence across save/reload.

## Preconditions

- Shotcut built and running: `cmake --build build && cmake --install build && ./run.sh`
- ControlServer on `ws://127.0.0.1:3847`
- Test project: `testdata/mcp-test.mlt` with a clip on track 0 (~3436 frames)

## Steps (agent-driven)

1. **List presets** — `shotcut_list_speed_presets` → 6 presets: `montage`, `hero`, `bullet`, `jump_cut`, `flash_in`, `flash_out`
2. **Open project** — `shotcut_open_project` with `testdata/mcp-test.mlt`
3. **Apply preset** — `shotcut_apply_speed_preset`:
   - `trackIndex: 0`, `clipIndex: 0`, `presetId: "hero"` → ok
4. **Read curve** — `shotcut_get_speed_curve`:
   - `present: true`; keyframes at frames `0` / `1374` / `2061` / `3435` with speeds `2` / `0.4` / `0.4` / `2`
5. **Constant speed** — `shotcut_apply_constant_speed`:
   - `speed: 2.0`, `pitchCompensation: true` → flat 2× curve, `pitch: 1`
6. **Custom keyframes** — `shotcut_set_speed_keyframes`:
   - custom ramp `1 → 3 → 1` → keyframes at frames `0` / `1718` / `3435`
7. **Persist** — reapply `hero` preset, `shotcut_save_project` → `testdata/out-speed.mlt`, reopen → hero curve persisted
8. **Remove** — `shotcut_remove_speed` → `present: false`

## Implementation notes

- Presets map to `timeremap` `speed_map` keyframes on the clip producer.
- **Known behavior (v1):** clip duration stayed **fixed** at 3436 frames after speed curve changes. Timeremap remaps source frames within the segment; v1 does not grow or shrink clip length on the timeline.
- MCP and UI both route through `EditorService` speed methods + Speed dock.

## UI path (manual)

View → Speed dock (Ctrl+Shift+5). Pick preset or set constant/custom speed on selected clip.

## Result

Hero preset applies a ramped speed curve visible in preview and `get_speed_curve` readback; constant and custom keyframes overwrite the map; save/reload preserves the last saved curve; `remove_speed` clears timeremap.

## Recording

Screen capture of preset apply + MCP `get_speed_curve` readback: `docs/recordings/06-speed-curve.mp4` (add locally).
