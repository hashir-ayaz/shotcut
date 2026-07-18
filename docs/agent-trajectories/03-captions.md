# Agent Trajectory — Captions (Phase 2.3)

## Goal

Import SRT captions via MCP, read cues, edit text, confirm subtitle track in state.

## Preconditions

- Shotcut running
- Sample SRT: `testdata/sample-captions.srt`
- **Whisper STT:** `whisper-cli` not installed on this machine; auto-transcribe path is implemented but requires Whisper binary + model (see `mcp-shotcut/README.md`). Import-SRT path works without Whisper.

## Steps (agent-driven)

1. **Import captions** — `shotcut_import_captions`:
   - `path: "testdata/sample-captions.srt"`
   - → `{ ok: true, trackName: "Imported Captions" }`
2. **Get captions** — `shotcut_get_captions` → 3 cues with start/end ms and text
3. **Edit cue** — `shotcut_set_caption_text`:
   - `trackIndex: 0`, `cueIndex: 0`, `text: "Hello from MCP"`
4. **Verify** — `shotcut_get_captions` shows updated first cue
5. **State** — `shotcut_get_state` includes `subtitleTrackCount >= 1`

## UI path

Subtitles dock → **Auto Captions (API)...** calls `EditorService::transcribeCaptions` (async Whisper job when configured). Text edits in Subtitles dock route through `setCaptionText` for MCP parity.

## Async transcription (optional)

When Whisper is configured in Shotcut Settings:

```
shotcut_transcribe_captions { language: "en" }
shotcut_get_job_status { jobId: "<label from response>" }
```

## Result

Subtitle track with editable cues; burn-in via existing Shotcut subtitle filter on export.

## Recording

`docs/recordings/03-captions.mp4` (add locally).

## Live demo session — 2026-07-18 (MCP)

Burned-in subtitles on the TED talk. Whisper auto-transcription was attempted first and returned an error verbatim, then the deterministic SRT-import path was used.

1. `shotcut_open_project` `testdata/blank-1080.mlt` → clear timeline; `shotcut_get_state` → `subtitleTrackCount:0`.
2. `shotcut_add_clip` `ted-talk-90s.mp4` `{ inPoint:0, append:true }` → clip 0 (frames 0–2696, full ~90s).
3. `shotcut_transcribe_captions` `{ language:"en", trackName:"Auto Captions" }` → **`ok:false`**, `error: "Whisper is not configured; set whisper executable and model in Settings"`.
4. `shotcut_import_captions` `{ path:".../ted-talk-90s.en.srt", trackName:"English" }` → `ok:true`, **`burnInApplied:true`**.
5. `shotcut_get_state` → `subtitleTrackCount:1`.
6. `shotcut_get_captions` → track `"English"` (lang `eng`), **255 cues**. First cue `13.24s–15.80s`: *"A few years ago, I broke into my own house."*

**Confirmed:** subtitle track created and burned into the preview (`burnInApplied:true`). Scrub ~frame 397 (13.2s) for the opening caption over the talking head.

**Caveat:** the supplied SRT is the full ~12-min talk (cues to ~735s), while the clip is the 90s excerpt — so only cues up to ~90s overlay video. Auto-transcription (Whisper) is unavailable on this build; live captions would require bundling/configuring `whisper-cli` + a model.
