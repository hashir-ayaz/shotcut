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

export function registerAnimationTools(
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
    "shotcut_list_animation_presets",
    { description: "List CapCut-style clip animation presets", inputSchema: {} },
    async () => formatResult(await client.request("list_animation_presets")),
  );

  server.registerTool(
    "shotcut_apply_clip_animation",
    {
      description: "Apply a clip animation preset (in, out, or combo)",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        presetId: z.string(),
        durationFrames: z.number().int(),
        mode: z.enum(["in", "out", "combo"]),
      },
    },
    async (args) =>
      formatResult(
        await client.request("apply_clip_animation", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
          presetId: args.presetId,
          durationFrames: args.durationFrames,
          mode: args.mode,
        }),
      ),
  );
}
