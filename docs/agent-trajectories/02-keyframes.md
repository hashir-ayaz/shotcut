# Agent Trajectory — Keyframes (Phase 2.2)

## Goal

Add opacity keyframes on a clip via MCP; confirm animation metadata and list readback.

## Preconditions

- Shotcut running with a clip on track 0 (e.g. after transitions test project)
- ControlServer on port 3847

## Steps (agent-driven)

1. **List animatable properties** — `shotcut_list_animatable_properties`:
   - `trackIndex: 0`, `clipIndex: 0`
   - Returns: `position.x`, `position.y`, `scale`, `rotation`, `opacity`
2. **Add keyframes** — `shotcut_add_keyframe` on `opacity`:
   - Frame `0`, value `1.0`
   - Frame `60`, value `0.3`
3. **List keyframes** — `shotcut_list_keyframes` → two keyframes with linear interpolation
4. **Save project** — persists in `.mlt` via existing filter/MLT keyframe storage

## Implementation notes

- Maps CapCut Basic panel → Shotcut filters:
  - Position/scale/rotation → `affineSizePosition` (`transition.rect`, `transition.fix_rotate_x`)
  - Opacity → `brightnessOpacity` (`alpha`)
- Uses `QmlFilter::set` with keyframe types; filters auto-attached via `ensureShotcutFilter`

## UI path

Existing Filters panel + Keyframes dock; property mutations can go through the same `EditorService` methods for MCP parity.

## Result

Opacity fades from 100% to 30% over 60 frames in preview. MCP `list_keyframes` matches filter state.

## Recording

`docs/recordings/02-keyframes.mp4` (add locally).
