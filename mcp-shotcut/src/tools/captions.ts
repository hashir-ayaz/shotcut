import { z } from "zod";

import type { ShotcutClient } from "../shotcutClient.js";

function formatResult(result: Awaited<ReturnType<ShotcutClient["request"]>>) {
  return {
    content: [
      {
        type: "text" as const,
        text: JSON.stringify(result, null, 2),
      },
    ],
  };
}

export function registerCaptionTools(
  server: {
    registerTool: (
      name: string,
      config: { description: string; inputSchema: Record<string, z.ZodTypeAny> },
      handler: (args: Record<string, unknown>) => Promise<{ content: { type: "text"; text: string }[] }>,
    ) => void;
  },
  client: ShotcutClient,
) {
  server.registerTool(
    "shotcut_transcribe_captions",
    {
      description: "Start Whisper auto-caption transcription",
      inputSchema: {
        language: z.string().optional(),
        translateToEnglish: z.boolean().optional(),
        trackName: z.string().optional(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("transcribe_captions", {
          language: args.language ?? "en",
          translateToEnglish: args.translateToEnglish ?? false,
          trackName: args.trackName,
        }),
      ),
  );

  server.registerTool(
    "shotcut_get_job_status",
    {
      description: "Get async job status by jobId label",
      inputSchema: {
        jobId: z.string(),
      },
    },
    async (args) => formatResult(await client.request("get_job_status", { jobId: args.jobId })),
  );

  server.registerTool(
    "shotcut_get_captions",
    { description: "Get subtitle tracks and cues", inputSchema: {} },
    async () => formatResult(await client.request("get_captions")),
  );

  server.registerTool(
    "shotcut_set_caption_text",
    {
      description: "Edit caption cue text",
      inputSchema: {
        trackIndex: z.number().int(),
        cueIndex: z.number().int(),
        text: z.string(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("set_caption_text", {
          trackIndex: args.trackIndex,
          cueIndex: args.cueIndex,
          text: args.text,
        }),
      ),
  );

  server.registerTool(
    "shotcut_import_captions",
    {
      description: "Import captions from an SRT file",
      inputSchema: {
        path: z.string(),
        trackName: z.string().optional(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("import_captions", {
          path: args.path,
          trackName: args.trackName,
        }),
      ),
  );
}
