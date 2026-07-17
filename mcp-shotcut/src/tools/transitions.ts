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

export function registerTransitionTools(
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
    "shotcut_list_transition_presets",
    { description: "List CapCut-style transition presets", inputSchema: {} },
    async () => formatResult(await client.request("list_transition_presets")),
  );

  server.registerTool(
    "shotcut_apply_transition",
    {
      description: "Apply a transition preset between clips",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        presetId: z.string(),
        durationFrames: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("apply_transition", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
          presetId: args.presetId,
          durationFrames: args.durationFrames,
        }),
      ),
  );

  server.registerTool(
    "shotcut_set_transition_duration",
    {
      description: "Set transition duration in frames",
      inputSchema: {
        trackIndex: z.number().int(),
        transitionClipIndex: z.number().int(),
        durationFrames: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("set_transition_duration", {
          trackIndex: args.trackIndex,
          transitionClipIndex: args.transitionClipIndex,
          durationFrames: args.durationFrames,
        }),
      ),
  );

  server.registerTool(
    "shotcut_get_transition",
    {
      description: "Get transition details for a transition clip",
      inputSchema: {
        trackIndex: z.number().int(),
        transitionClipIndex: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("get_transition", {
          trackIndex: args.trackIndex,
          transitionClipIndex: args.transitionClipIndex,
        }),
      ),
  );

  server.registerTool(
    "shotcut_remove_transition",
    {
      description: "Remove a transition clip",
      inputSchema: {
        trackIndex: z.number().int(),
        transitionClipIndex: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("remove_transition", {
          trackIndex: args.trackIndex,
          transitionClipIndex: args.transitionClipIndex,
        }),
      ),
  );
}
