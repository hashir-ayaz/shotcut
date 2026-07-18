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

export function registerAudioTools(
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
    "shotcut_apply_audio_denoise",
    {
      description: "Apply RNNoise audio denoise to a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        amount: z.number().optional(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("apply_audio_denoise", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
          amount: args.amount ?? 1,
        }),
      ),
  );

  server.registerTool(
    "shotcut_get_audio_denoise",
    {
      description: "Get audio denoise settings for a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("get_audio_denoise", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
        }),
      ),
  );

  server.registerTool(
    "shotcut_remove_audio_denoise",
    {
      description: "Remove audio denoise from a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("remove_audio_denoise", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
        }),
      ),
  );
}
