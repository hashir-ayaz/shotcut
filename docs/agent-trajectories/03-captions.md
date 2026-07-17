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
