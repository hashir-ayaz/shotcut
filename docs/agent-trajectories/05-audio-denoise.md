# Agent Trajectory — Audio Denoise (Phase 2.5)

## Goal

Apply CapCut-style audio denoise via MCP, inspect readback, remove, and confirm the filter survives save/reload.

## Preconditions

- Shotcut built and running: `cmake --build build && cmake --install build && ./run.sh`
- ControlServer on `ws://127.0.0.1:3847`
- Test project: `testdata/mcp-test.mlt` with an audio clip on track 0

## Steps (agent-driven)

1. **Ping** — `shotcut_ping` → `{ ok: true, data: { pong: true } }`
2. **Open project** — `shotcut_open_project` with `testdata/mcp-test.mlt`
3. **Apply denoise** — `shotcut_apply_audio_denoise`:
   - `trackIndex: 0`, `clipIndex: 0`, `amount: 1.0`
   - → ok; service `avfilter.arnndn` (Homebrew MLT 7.40 has no `rnnoise` link)
4. **Read back** — `shotcut_get_audio_denoise`:
   - `present: true`, `amount: 1`, `service: avfilter.arnndn`
5. **Update strength** — `shotcut_apply_audio_denoise` with `amount: 0.75` → amount updates
6. **Remove** — `shotcut_remove_audio_denoise` → `present: false`
7. **Persist** — reapply `amount: 0.9`, `shotcut_save_project` → `testdata/out-denoise.mlt`, reopen → `present: true`, `amount: 0.9`

## Implementation notes

- Homebrew MLT 7.40 does not ship the `rnnoise` link (no `librnnoise`).
- `EditorService` prefers native `rnnoise` Link when MLT provides it; otherwise falls back to FFmpeg `avfilter.arnndn` (RNN speech denoise) with `av.mix` as strength control.
- MCP and UI both route through `EditorService::applyAudioDenoise` / `getAudioDenoise` / `removeAudioDenoise`.

## UI path (manual)

View → Audio dock (Ctrl+Shift+6). Set strength %, select clip, **Reduce Noise on Selected Clip**.

## Result

Denoise filter attached to clip audio; MCP readback matches filter state; survives save/reload. On this build, backend is `avfilter.arnndn` rather than native RNNoise.

## Recording

Screen capture of UI apply + MCP `get_audio_denoise` readback: `docs/recordings/05-audio-denoise.mp4` (add locally).
