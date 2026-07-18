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
