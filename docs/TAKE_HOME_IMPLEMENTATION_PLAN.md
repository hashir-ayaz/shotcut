# Shotcut Take-Home — Implementation Plan

> **For agentic workers:** Implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Prefer one feature slice at a time: **Editor API → thin UI → MCP tool → agent evidence**.

**Goal:** Ship 3 CapCut-style Shotcut features end-to-end, each backed by a single C++ Editor API used by both the UI and a TypeScript MCP server, then prove a Cursor agent can drive them via MCP (trajectories + trimmed recordings) and produce an HTML writeup.

**Architecture:** All edit/inspect business logic lives in an in-process C++ `EditorService`. The Qt/QML UI is a thin shell that only calls that service (so computer-use and MCP stay equivalent). A small JSON-RPC WebSocket `ControlServer` inside Shotcut is a transport adapter only — no duplicate logic. A TypeScript MCP package wraps that protocol for Cursor.

**Tech Stack:** Shotcut (Qt 6 / C++ / MLT), local JSON-RPC over WebSocket, TypeScript MCP SDK (`@modelcontextprotocol/sdk`), Cursor agent for evidence.

## Decisions (locked)

| Decision | Choice |
|----------|--------|
| Scope | **3 features done well** (not all six) |
| Backend | In-process C++ **Editor API** as sole mutation/inspect path |
| UI | Thin shell over Editor API (computer-use parity) |
| MCP | TypeScript process talking to Shotcut over local WebSocket JSON-RPC |
| Agent | Cursor agent |
| Plan path | `docs/TAKE_HOME_IMPLEMENTATION_PLAN.md` |

## Feature selection (must-ship)

Prioritized for existing Shotcut seams + CapCut demo impact + export-visible results:

| Priority | Feature | Why |
|----------|---------|-----|
| **P0** | Transitions (dissolve + wipe family, duration) | `MultitrackModel::addTransition*`, `AddTransitionCommand`, luma/mix UI already exist — CapCut UX + API + MCP |
| **P0** | Keyframes (position, scale, rotation, opacity) | KeyframesDock / KeyframesModel / `QmlFilter` already strong — expose via Editor API + CapCut-style gaps |
| **P0** | Captions (auto from audio) | Whisper job + SubtitlesDock exist — wire through Editor API + MCP |
| Stretch | Clip animation presets (In/Out/Combo) | Thin presets on top of filters/keyframes if time |
| Out of scope for v1 | Speed curve, stem separation | Higher MLT/ML cost; document as skipped |

Be explicit in the writeup what is complete vs skipped.

## Global constraints

- Public fork of Shotcut; buildable on this machine via `LOCAL_SETUP.md` / `./run.sh`.
- **Never put edit logic in MCP or QML** — only call `EditorService`.
- Every shipped feature must have: UI path + MCP tools + inspect/readback + Cursor trajectory + trimmed recording.
- After every `cmake --install`, run `./run.sh` (Homebrew MLT paths).
- Match CapCut **capability and export result**, not pixel-perfect panels.
- Timebox honesty: prefer finishing the MCP + agent loop on 3 features over starting more.

## Architecture

```text
┌─────────────────────────────────────────────────────────────────┐
│ Shotcut.app                                                      │
│                                                                  │
│   Thin UI (docks / QML / actions)                                │
│            │                                                     │
│            ▼                                                     │
│   ┌─────────────────────────────────────────┐                    │
│   │ EditorService  (C++ — ONLY business logic)│                    │
│   │  open/save, timeline, transitions,       │                    │
│   │  keyframes, captions, state snapshots    │                    │
│   │  → MultitrackModel / commands / jobs     │                    │
│   └──────────────────▲──────────────────────┘                    │
│                      │                                           │
│   ┌──────────────────┴──────────────────────┐                    │
│   │ ControlServer (QWebSocketServer)         │                    │
│   │ JSON-RPC 2.0 adapter → EditorService     │                    │
│   └──────────────────────────────────────────┘                    │
└───────────────────────────────┬──────────────────────────────────┘
                                │ ws://127.0.0.1:<port>
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│ mcp-shotcut/ (TypeScript)                                        │
│   MCP tools ↔ JSON-RPC client ↔ ControlServer                    │
│   Used by Cursor agent                                           │
└─────────────────────────────────────────────────────────────────┘
```

**Parity rule:** If an agent can do X via MCP, a human (or computer-use) must be able to do X by clicking UI that calls the same `EditorService` method.

**Why a WebSocket exists despite “in-process API”:** TypeScript MCP cannot link into the Shotcut process. The socket is a **thin transport**, not a second backend. Handlers must be one-liners into `EditorService`.

---

## File map (create / touch)

### New — Editor API + control plane

| Path | Responsibility |
|------|----------------|
| `src/services/editorservice.h` / `.cpp` | Single facade for open/save, clips, transitions, keyframes, captions, `getState()` |
| `src/services/editortypes.h` | Shared structs / result types (`EditorResult`, `TransitionSpec`, `KeyframeSpec`, …) |
| `src/services/controlserver.h` / `.cpp` | `QWebSocketServer` + JSON-RPC dispatch → `EditorService` |
| `src/services/CMakeLists.txt` or entries in `src/CMakeLists.txt` | Wire new sources into `shotcut` target |

### New — TypeScript MCP

| Path | Responsibility |
|------|----------------|
| `mcp-shotcut/package.json` | MCP server package |
| `mcp-shotcut/tsconfig.json` | TypeScript config |
| `mcp-shotcut/src/index.ts` | MCP server entry (stdio) |
| `mcp-shotcut/src/shotcutClient.ts` | WebSocket JSON-RPC client |
| `mcp-shotcut/src/tools/*.ts` | One file per tool group |
| `mcp-shotcut/README.md` | How to launch against local Shotcut build |

### New — docs / evidence / deliverable

| Path | Responsibility |
|------|----------------|
| `docs/TAKE_HOME_IMPLEMENTATION_PLAN.md` | This plan |
| `docs/agent-trajectories/` | One markdown JSON transcript per feature |
| `docs/recordings/` | Trimmed screen recordings (or links) |
| `docs/take_home_writeup.html` | Final ~1-page visual writeup |

### Existing — primary integration points

| Path | Role |
|------|------|
| `src/mainwindow.cpp` / `.h` | Construct `EditorService` + start `ControlServer`; route menu/open/save through service over time |
| `src/models/multitrackmodel.*` | Transitions / timeline mutations |
| `src/commands/timelinecommands.*` | Undoable transition commands |
| `src/docks/timelinedock.*` | Thin UI for transitions |
| `src/docks/keyframesdock.*`, `src/models/keyframesmodel.*`, `src/qmltypes/qmlfilter.*` | Keyframes |
| `src/docks/subtitlesdock.*`, `src/jobs/whisperjob.*`, `src/dialogs/transcribeaudiodialog.*` | Captions |
| `src/widgets/lumamixtransition.*` | Transition properties UI |
| `run.sh`, `LOCAL_SETUP.md` | Local Homebrew launch |

---

## Phase 0 — Local readiness (before feature work)

### Task 0.1: Verify build + edit a clip

**Files:** none (ops only)

- [ ] **Step 1:** Confirm app exists and launches with MLT plugins

```bash
cd ~/development/shotcut
./run.sh
# or: open install/Shotcut.app
```

Check log for **absence** of `no plugins found in .../mlt-7`:

```bash
rg "no plugins found|MLT_REPOSITORY" \
  ~/Library/Application\ Support/Meltytech/Shotcut/shotcut-log.txt | tail -20
```

- [ ] **Step 2:** Download sample footage (speech + cuts)

```bash
mkdir -p ~/development/shotcut/testdata
yt-dlp -f "bv*+ba/b" -o "~/development/shotcut/testdata/sample.%(ext)s" \
  "https://www.youtube.com/watch?v=VIDEO_ID"
```

- [ ] **Step 3:** Manually import clip, put on timeline, scrub, save a `.mlt` project. Confirm open/save works before coding.

- [ ] **Step 4:** Note working project path for later MCP tests (e.g. `testdata/demo.mlt`).

---

## Phase 1 — MCP foundation (do this first)

Goal: Cursor can connect to a running Shotcut, call `ping` / `get_state`, and get structured JSON — **before** any CapCut feature work.

### Task 1.1: `EditorService` skeleton + `getState` / project open-save

**Files:**
- Create: `src/services/editortypes.h`
- Create: `src/services/editorservice.h`
- Create: `src/services/editorservice.cpp`
- Modify: `src/CMakeLists.txt` (add sources)
- Modify: `src/mainwindow.h` / `src/mainwindow.cpp` (own singleton accessor or member)

**Interfaces:**
- Produces:
  - `EditorService::instance()` (or `MAIN.editor()`)
  - `EditorResult { bool ok; QString error; QJsonObject data; }`
  - `EditorResult ping()`
  - `EditorResult getState()`
  - `EditorResult openProject(const QString &path)`
  - `EditorResult saveProject(const QString &path)`
  - `EditorResult addClipToTimeline(const QString &path, int trackIndex, int position)`

**Design notes:**
- `getState()` returns enough for an agent to orient: project path, profile (fps/resolution), track count, clips (`track`, `index`, `name`, `in`, `out`, `length`, `isTransition`), selection, playhead position, subtitle track count.
- Prefer calling existing `MainWindow` / `MultitrackModel` / `MLT` APIs rather than reimplementing MLT XML.
- All public methods must be callable from the GUI thread (ControlServer will marshal to GUI thread via `QMetaObject::invokeMethod` if needed).

- [ ] **Step 1:** Add `editortypes.h` with `EditorResult` and JSON helpers (`toJson()`).

- [ ] **Step 2:** Implement `EditorService` with `ping`, `getState`, `openProject`, `saveProject`, `addClipToTimeline` delegating to existing MainWindow/MLT paths.

- [ ] **Step 3:** Construct service from `MainWindow` startup; expose via accessor.

- [ ] **Step 4:** Build + install + `./run.sh`

```bash
cmake --build build -j"$(sysctl -n hw.ncpu)"
cmake --install build
./run.sh
```

- [ ] **Step 5:** Temporary debug: call `getState()` from a menu action or `qDebug` on startup to verify non-empty state after opening a project.

### Task 1.2: `ControlServer` JSON-RPC WebSocket

**Files:**
- Create: `src/services/controlserver.h`
- Create: `src/services/controlserver.cpp`
- Modify: `src/mainwindow.cpp` (start server when app is ready)
- Modify: settings or env: `SHOTCUT_MCP_PORT` (default `3847`)

**Protocol (JSON-RPC 2.0 over WebSocket text frames):**

Request:
```json
{"jsonrpc":"2.0","id":1,"method":"ping","params":{}}
```

Success:
```json
{"jsonrpc":"2.0","id":1,"result":{"ok":true,"data":{"pong":true,"version":"..."}}}
```

Error:
```json
{"jsonrpc":"2.0","id":1,"error":{"code":-32000,"message":"..."}}
```

**Method map (Phase 1):**

| RPC method | EditorService |
|------------|---------------|
| `ping` | `ping()` |
| `get_state` | `getState()` |
| `open_project` | `openProject(path)` |
| `save_project` | `saveProject(path)` |
| `add_clip` | `addClipToTimeline(path, trackIndex, position)` |

**Rules:**
- Dispatch **only** to `EditorService` — no timeline math in the server.
- Bind `127.0.0.1` only.
- Log listen port at info level so MCP README can say “look for ControlServer listening on …”.
- Use `Qt::QueuedConnection` / invoke on GUI thread for all EditorService calls.

- [ ] **Step 1:** Implement `ControlServer::start(quint16 port)`.

- [ ] **Step 2:** Parse JSON-RPC, dispatch, write response.

- [ ] **Step 3:** Start from `MainWindow` after UI init; read `qEnvironmentVariableIntValue("SHOTCUT_MCP_PORT", &ok)` default `3847`.

- [ ] **Step 4:** Manual test with `websocat` or a tiny node script:

```bash
# example with websocat if installed
echo '{"jsonrpc":"2.0","id":1,"method":"ping","params":{}}' | websocat ws://127.0.0.1:3847
```

Expected: `result.ok == true`.

### Task 1.3: TypeScript MCP package scaffolding

**Files:**
- Create: `mcp-shotcut/package.json`
- Create: `mcp-shotcut/tsconfig.json`
- Create: `mcp-shotcut/src/shotcutClient.ts`
- Create: `mcp-shotcut/src/index.ts`
- Create: `mcp-shotcut/README.md`

**Interfaces:**
- Produces MCP tools (Phase 1):
  - `shotcut_ping`
  - `shotcut_get_state`
  - `shotcut_open_project` `{ path: string }`
  - `shotcut_save_project` `{ path: string }`
  - `shotcut_add_clip` `{ path: string, trackIndex?: number, position?: number }`
- Env: `SHOTCUT_WS_URL` default `ws://127.0.0.1:3847`

- [ ] **Step 1:** Init package with `@modelcontextprotocol/sdk`, `ws`, `zod`, `typescript`.

- [ ] **Step 2:** Implement `ShotcutClient.request(method, params)`.

- [ ] **Step 3:** Register Phase 1 tools in `index.ts` (stdio transport for Cursor).

- [ ] **Step 4:** Document Cursor MCP config snippet in `mcp-shotcut/README.md`:

```json
{
  "mcpServers": {
    "shotcut": {
      "command": "npx",
      "args": ["tsx", "/Users/hashirayaz/development/shotcut/mcp-shotcut/src/index.ts"],
      "env": { "SHOTCUT_WS_URL": "ws://127.0.0.1:3847" }
    }
  }
}
```

- [ ] **Step 5:** Smoke test: launch Shotcut → Cursor agent calls `shotcut_ping` and `shotcut_get_state`.

**Exit criteria for Phase 1:** Agent can open a project, add a clip, save, and read state back. No CapCut features yet — foundation only.

---

## Phase 2 — Feature slices (API + thin UI + MCP together)

For **each** feature below, follow this order — do not build UI-only or MCP-only paths:

1. Add `EditorService` methods (+ undo via existing commands)
2. Point / add thin UI to those methods
3. Add ControlServer RPC methods
4. Add TypeScript MCP tools
5. Manual UI test + MCP test + export check
6. Cursor agent trajectory + trimmed recording

---

### Task 2.1: Transitions (P0)

**CapCut “done”:** Drag/apply dissolve + wipe family between two adjacent clips; adjustable duration; visible on export.

**Files:**
- Modify: `src/services/editorservice.*`
- Modify: `src/services/controlserver.*`
- Modify: `src/docks/timelinedock.*` and/or transition UI entry points
- Modify: `src/widgets/lumamixtransition.*` (only if needed for wipe family / duration)
- Modify: `mcp-shotcut/src/tools/transitions.ts`
- Leverage: `MultitrackModel::addTransition`, `AddTransitionCommand`, track transition service / luma wipes

**EditorService API:**

```cpp
EditorResult listTransitionPresets();
// returns [{ id, name, family: "dissolve"|"wipe", mltService, params }]

EditorResult applyTransition(int trackIndex, int clipIndex, const QString &presetId, int durationFrames);
EditorResult setTransitionDuration(int trackIndex, int transitionClipIndex, int durationFrames);
EditorResult getTransition(int trackIndex, int transitionClipIndex);
EditorResult removeTransition(int trackIndex, int transitionClipIndex);
```

**MCP tools:**
- `shotcut_list_transition_presets`
- `shotcut_apply_transition` `{ trackIndex, clipIndex, presetId, durationFrames }`
- `shotcut_set_transition_duration` `{ trackIndex, transitionClipIndex, durationFrames }`
- `shotcut_get_transition` `{ trackIndex, transitionClipIndex }`

**UI (thin):**
- CapCut-style: pick dissolve / wipe preset + duration, apply to selection or clip join.
- Buttons/menus call `EditorService::applyTransition` — no duplicate MLT logic in QML.

- [x] **Step 1:** Inventory existing wipe/dissolve MLT services available via current Homebrew MLT; map to preset IDs.

- [x] **Step 2:** Implement `listTransitionPresets` / `applyTransition` / duration / remove using existing model commands.

- [x] **Step 3:** Wire thin UI → service.

- [x] **Step 4:** RPC + MCP tools.

- [x] **Step 5:** Verify: UI apply → `get_state` shows transition; MCP apply → UI shows transition; export contains transition.

- [x] **Step 6:** Cursor agent trajectory → `docs/agent-trajectories/01-transitions.md` + recording under `docs/recordings/`.

---

### Task 2.2: Keyframes (P0)

**CapCut “done”:** Keyframe toggle + interpolation on animatable clip properties: at least **position (x/y), scale, rotation, opacity**.

**Files:**
- Modify: `src/services/editorservice.*`
- Modify: `src/services/controlserver.*`
- Modify: thin glue in `keyframesdock` / filter UI only as needed
- Modify: `mcp-shotcut/src/tools/keyframes.ts`
- Leverage: `KeyframesModel`, `QmlFilter`, size/position / opacity / rotate filters

**EditorService API:**

```cpp
EditorResult listAnimatableProperties(int trackIndex, int clipIndex);
// [{ propertyId, label, filterService, param }]

EditorResult addKeyframe(int trackIndex, int clipIndex, const QString &propertyId,
                         int frame, double value, const QString &interpolation);
EditorResult removeKeyframe(int trackIndex, int clipIndex, const QString &propertyId, int frame);
EditorResult listKeyframes(int trackIndex, int clipIndex, const QString &propertyId);
EditorResult setPropertyValue(int trackIndex, int clipIndex, const QString &propertyId, double value);
```

**MCP tools:**
- `shotcut_list_animatable_properties`
- `shotcut_add_keyframe`
- `shotcut_remove_keyframe`
- `shotcut_list_keyframes`
- `shotcut_set_clip_property`

**UI (thin):**
- Ensure CapCut-like diamond/keyframe affordance for the four properties (reuse Keyframes panel where possible).
- All mutations go through `EditorService` (dock methods become wrappers).

- [x] **Step 1:** Map CapCut properties → Shotcut filters/params (`affine` / size-position, opacity, rotate).

- [x] **Step 2:** Implement service methods on top of `QmlFilter` / `KeyframesModel` / filter commands.

- [x] **Step 3:** Thin UI wrappers.

- [x] **Step 4:** RPC + MCP.

- [x] **Step 5:** Verify keyframes animate in preview and survive save/reload/export.

- [x] **Step 6:** Trajectory + recording: `docs/agent-trajectories/02-keyframes.md`.

---

### Task 2.3: Captions / auto captions (P0)

**CapCut “done”:** Auto-generate captions from clip audio (disclose STT stack — Shotcut’s Whisper job). Optional stretch: apply one simple animated text template.

**Files:**
- Modify: `src/services/editorservice.*`
- Modify: `src/services/controlserver.*`
- Modify: thin wrappers around `SubtitlesDock` / Whisper launch
- Modify: `mcp-shotcut/src/tools/captions.ts`
- Leverage: `jobs/whisperjob.*`, `dialogs/transcribeaudiodialog.*`, `commands/subtitlecommands.*`

**EditorService API:**

```cpp
EditorResult transcribeCaptions(const QString &language, bool translateToEnglish);
EditorResult getCaptions();  // list cues: start, end, text, track
EditorResult setCaptionText(int trackIndex, int cueIndex, const QString &text);
EditorResult importCaptionsFile(const QString &srtPath);
// stretch:
EditorResult applyTextTemplate(const QString &templateId, const QString &text, int start, int end);
```

**MCP tools:**
- `shotcut_transcribe_captions`
- `shotcut_get_captions`
- `shotcut_set_caption_text`
- `shotcut_import_captions`

**UI (thin):**
- “Auto captions” action → `EditorService::transcribeCaptions` (Whisper job under the hood).
- Show progress via existing Jobs dock.

- [x] **Step 1:** Confirm Whisper/Kokorodoki prerequisites on this machine; document in MCP README / writeup.

- [x] **Step 2:** Wrap existing transcribe flow in `EditorService` (async job id returned in `data`).

- [x] **Step 3:** Add `getCaptions` / edit / import.

- [x] **Step 4:** RPC + MCP; include `shotcut_get_job_status` if async wait is needed for agents.

- [x] **Step 5:** Verify captions appear on timeline/subtitle track and export/burn-in path as supported by Shotcut today.

- [x] **Step 6:** Trajectory + recording: `docs/agent-trajectories/03-captions.md`.

---

### Task 2.4 (stretch): Clip animation presets

Only if P0 complete with evidence.

**EditorService:** `listAnimationPresets()`, `applyClipAnimation(track, clip, presetId, durationFrames, mode: in|out|combo)`.

Implement as presets that set filter + keyframe patterns via the same keyframe APIs (no parallel animation engine).

---

## Phase 3 — Agent evidence + writeup

### Task 3.1: Cursor agent test protocol

For each P0 feature, run the same script structure:

1. Start Shotcut (`./run.sh`) with ControlServer up.
2. Ensure Cursor MCP `shotcut` server is configured.
3. Prompt agent with a **closed task**, e.g.:

> Open `testdata/demo.mlt`. Apply a 24-frame dissolve between the first two clips on track 0. Call `shotcut_get_state` / `shotcut_get_transition` to confirm. Save to `testdata/out-transitions.mlt`.

4. Save full tool-call transcript to `docs/agent-trajectories/0N-*.md`.
5. Capture screen recording; trim to essential steps only → `docs/recordings/0N-*.mp4` (or `.mov`).

- [x] Transitions evidence
- [x] Keyframes evidence
- [x] Captions evidence

### Task 3.2: HTML writeup

**File:** `docs/take_home_writeup.html`

Must open in a browser with no build step. Include:

- What you built (complete vs skipped)
- CapCut → Shotcut mapping (screenshots OK)
- Editor API + MCP architecture diagram
- How to build/run + launch MCP
- Agent results (embed or link trajectories/recordings)
- Honest limitations (Whisper deps, wipe preset list, etc.)

### Task 3.3: Repo hygiene for submission

- [x] `mcp-shotcut/README.md` complete
- [ ] Root or docs note pointing to plan + writeup + trajectories
- [ ] `.gitignore` for `node_modules`, build artifacts; **do** commit `mcp-shotcut` sources and docs evidence
- [ ] Confirm clean rebuild from instructions on a fresh shell

---

## Suggested MCP tool surface (final)

| Tool | Phase |
|------|-------|
| `shotcut_ping` | 1 |
| `shotcut_get_state` | 1 |
| `shotcut_open_project` | 1 |
| `shotcut_save_project` | 1 |
| `shotcut_add_clip` | 1 |
| `shotcut_list_transition_presets` | 2.1 |
| `shotcut_apply_transition` | 2.1 |
| `shotcut_set_transition_duration` | 2.1 |
| `shotcut_get_transition` | 2.1 |
| `shotcut_list_animatable_properties` | 2.2 |
| `shotcut_add_keyframe` | 2.2 |
| `shotcut_remove_keyframe` | 2.2 |
| `shotcut_list_keyframes` | 2.2 |
| `shotcut_set_clip_property` | 2.2 |
| `shotcut_transcribe_captions` | 2.3 |
| `shotcut_get_captions` | 2.3 |
| `shotcut_set_caption_text` | 2.3 |
| `shotcut_import_captions` | 2.3 |
| `shotcut_get_job_status` | 2.3 (if async) |

Keep tools **one meaningful editor action each**, typed args, useful returns (`ok`, `error`, `data`).

---

## Build / run cheat sheet (every code change)

```bash
cd ~/development/shotcut
cmake --build build -j"$(sysctl -n hw.ncpu)"
cmake --install build
./run.sh   # REQUIRED — restores MLT env wrapper + CuteLogger
```

MCP (separate terminal):

```bash
cd ~/development/shotcut/mcp-shotcut
npm install
npx tsx src/index.ts   # or via Cursor mcpServers config
```

Logs: `~/Library/Application Support/Meltytech/Shotcut/shotcut-log.txt`

---

## Evaluation mapping (from the brief)

| They score | How this plan hits it |
|------------|----------------------|
| Feature quality | P0 features work in UI + export |
| Codebase navigation | Changes at `EditorService` + existing model/command seams |
| MCP design | Small typed TS tool surface over one RPC |
| Agent evidence | Trajectories + trimmed recordings per feature |
| Craft | Honest scope, reproducible `LOCAL_SETUP` + MCP README + HTML writeup |

---

## Order of execution (checklist)

- [ ] **Phase 0** — Build, sample footage, manual edit
- [ ] **Phase 1.1** — `EditorService` skeleton
- [ ] **Phase 1.2** — `ControlServer` JSON-RPC
- [ ] **Phase 1.3** — TypeScript MCP + Cursor ping/state
- [x] **Phase 2.1** — Transitions (API → UI → MCP → evidence)
- [x] **Phase 2.2** — Keyframes (API → UI → MCP → evidence)
- [x] **Phase 2.3** — Captions (API → UI → MCP → evidence)
- [ ] **Phase 2.4** — Stretch animations only if time
- [ ] **Phase 3** — Writeup + submission polish

---

## Out of scope / explicit skips (document in writeup)

- Full CapCut pixel-perfect panels
- Non-linear speed-curve engine (P1 skip unless stretch after P0)
- ML stem separation (vocals/instruments)
- Computer-use as the primary agent path (MCP is primary; UI parity enables computer-use equivalence)
- Refactoring all of MainWindow onto EditorService (migrate call sites incrementally for shipped features only)

---

## Spec coverage check

| Brief requirement | Plan task |
|-------------------|-----------|
| Build Shotcut / edit a clip first | 0.1 |
| CapCut-style features | 2.1–2.3 (+ 2.4 stretch) |
| Prioritize quality over all six | Feature selection table |
| MCP server + tools for shipped features | 1.3 + 2.x MCP steps |
| Agent trajectories | 3.1 |
| Trimmed recordings | 3.1 |
| HTML writeup | 3.2 |
| Public fork / reproducible | 3.3 + LOCAL_SETUP |

---

## Execution handoff

Plan saved to `docs/TAKE_HOME_IMPLEMENTATION_PLAN.md`.

When you want to implement:

1. **Subagent-driven** — one task per subagent with review between tasks (best for Phase 1 scaffolding + each feature slice)
2. **Inline in this session** — execute sequentially with checkpoints after Phase 1 and each P0 feature

Which approach do you want to start with?
