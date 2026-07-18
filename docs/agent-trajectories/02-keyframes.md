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

## Live demo session — 2026-07-18 (MCP)

Keyframes in **multiple places** — three clips, four properties, all smooth interpolation — on the transitions timeline (real clips at indices 0, 2, 4; transitions at 1 and 3).

1. `shotcut_list_animatable_properties` `{ trackIndex:0, clipIndex:0 }` → `position.x`, `position.y`, `scale`, `rotation`, `opacity`.
2. **Clip 0** (`bbb`, 0–240) — Ken Burns zoom: `shotcut_add_keyframe` `scale` `0→1.0`, `240→1.5`.
3. **Clip 2** (`ted`, 301–541) — pan there-and-back: `shotcut_add_keyframe` `position.x` `0→0`, `120→-350`, `240→0`.
4. **Clip 4** (`bbb`, 602–902) — spin: `shotcut_add_keyframe` `rotation` `0→0`, `300→360`.
5. **Clip 4** — fade-out: `shotcut_add_keyframe` `opacity` `210→1.0`, `300→0.0`.
6. Read-backs via `shotcut_list_keyframes`:

| Clip | Property | Keyframes (frame→value) |
|---|---|---|
| 0 | `scale` | 0→1.0, 240→1.5 |
| 2 | `position.x` | 0→0, 120→-350, 240→0 |
| 4 | `rotation` | 0→0, 300→360 |
| 4 | `opacity` | 210→1.0, 300→0.0 |

**Confirmed:** every value echoed back exactly. Clip 4 carries **two keyframed properties at once** (spin + fade) layered on top of its Hero speed ramp — keyframes, speed, and transitions coexist on one timeline.
