# Agent Trajectory — Clip Animations (Phase 2.4)

## Goal

Apply CapCut-style clip animation presets (fade in, slide out) via MCP; confirm keyframe readback and save.

## Preconditions

- Shotcut running with a clip on track 0 (e.g. `testdata/blank-1080.mlt` or transitions demo project)
- ControlServer on port 3847

## Steps (agent-driven)

1. **List presets** — `shotcut_list_animation_presets`
   - Returns six presets: `fade`, `zoom_in`, `zoom_out`, `slide_left`, `slide_up`, `fade_zoom`
2. **Apply fade in** — `shotcut_apply_clip_animation`:
   - `trackIndex: 0`, `clipIndex: 0`, `presetId: "fade"`, `durationFrames: 24`, `mode: "in"`
3. **Apply slide out** — `shotcut_apply_clip_animation`:
   - Same clip, `presetId: "slide_left"`, `mode: "out"`
4. **Read back** — `shotcut_list_keyframes` on `opacity` and `position.x` shows expected frame windows
5. **Save project** — keyframes persist in `.mlt` via existing filter storage

## Implementation notes

- Thin `EditorService` layer over `addKeyframe`; no parallel animation engine
- Presets map to `brightnessOpacity` (opacity) and `affineSizePosition` (scale/position)
- Modes: `in` (start of clip), `out` (end), `combo` (both; shrinks duration if clip is short)
- UI: **View → Animations** dock (Ctrl+Shift+7), tabbed with Filters/Transitions

## Result

Preview shows fade-in at clip start and slide-left at clip end. MCP `list_keyframes` returns rect-derived scale/position values.

## Recording

`docs/recordings/04-clip-animations.mp4` (add locally).

## Live demo session — 2026-07-18 (MCP)

One clip per **mode** — In, Out, Combo — on the captions timeline (ted clip + two appended `bbb` clips).

1. `shotcut_list_animation_presets` → 6 presets, each supporting `in`/`out`/`combo`.
2. `shotcut_add_clip` `bbb-60s.mp4` `{ inPoint:0, outPoint:150, append:true }` → clip 1 (frames 2697–2847).
3. `shotcut_add_clip` `bbb-60s.mp4` `{ inPoint:150, outPoint:300, append:true }` → clip 2 (frames 2848–2998).
4. **IN** — `shotcut_apply_clip_animation` `{ clipIndex:0, presetId:"fade", durationFrames:30, mode:"in" }` → `appliedProperties:["opacity"]`.
5. **OUT** — `shotcut_apply_clip_animation` `{ clipIndex:1, presetId:"slide_left", durationFrames:30, mode:"out" }` → `appliedProperties:["position.x"]`.
6. **COMBO** — `shotcut_apply_clip_animation` `{ clipIndex:2, presetId:"fade_zoom", durationFrames:30, mode:"combo" }` → `appliedProperties:["opacity","scale"]`.
7. Read-backs via `shotcut_list_keyframes`:

| Clip | Mode · Preset | Keyframes |
|---|---|---|
| 0 ted | IN · fade | `opacity` 0→0.0, 30→1.0 |
| 1 bbb | OUT · slide_left | `position.x` 121→0, 150→-1920 |
| 2 bbb | COMBO · fade_zoom | `opacity` 0→0, 30→1, 121→1, 150→0 · `scale` 0→0.85, 30→1.0, 121→1.0, 150→1.15 |

**Confirmed:** IN writes keyframes only at the head, OUT only at the tail, COMBO at both ends (the 4-keyframe curves on clip 2). `slide_left` moves `position.x` to `-1920` (full frame width) so the clip exits completely; `fade_zoom` combo layers two properties.
