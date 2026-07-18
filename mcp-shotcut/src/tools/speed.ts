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

const speedKeyframeSchema = z.object({
  position: z.number().optional(),
  frame: z.number().int().optional(),
  speed: z.number(),
  interpolation: z.string().optional(),
});

export function registerSpeedTools(
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
    "shotcut_list_speed_presets",
    { description: "List CapCut-style speed presets", inputSchema: {} },
    async () => formatResult(await client.request("list_speed_presets")),
  );

  server.registerTool(
    "shotcut_apply_speed_preset",
    {
      description: "Apply a speed preset to a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        presetId: z.string(),
        pitchCompensation: z.boolean().optional(),
      },
    },
    async (args) => {
      const params: Record<string, unknown> = {
        trackIndex: args.trackIndex,
        clipIndex: args.clipIndex,
        presetId: args.presetId,
      };
      if (args.pitchCompensation !== undefined) params.pitchCompensation = args.pitchCompensation;
      return formatResult(await client.request("apply_speed_preset", params));
    },
  );

  server.registerTool(
    "shotcut_apply_constant_speed",
    {
      description: "Apply a constant speed multiplier to a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        speed: z.number(),
        pitchCompensation: z.boolean().optional(),
      },
    },
    async (args) => {
      const params: Record<string, unknown> = {
        trackIndex: args.trackIndex,
        clipIndex: args.clipIndex,
        speed: args.speed,
      };
      if (args.pitchCompensation !== undefined) params.pitchCompensation = args.pitchCompensation;
      return formatResult(await client.request("apply_constant_speed", params));
    },
  );

  server.registerTool(
    "shotcut_set_speed_keyframes",
    {
      description: "Set custom speed keyframes on a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        keyframes: z.array(speedKeyframeSchema),
        pitchCompensation: z.boolean().optional(),
      },
    },
    async (args) => {
      const params: Record<string, unknown> = {
        trackIndex: args.trackIndex,
        clipIndex: args.clipIndex,
        keyframes: args.keyframes,
      };
      if (args.pitchCompensation !== undefined) params.pitchCompensation = args.pitchCompensation;
      return formatResult(await client.request("set_speed_keyframes", params));
    },
  );

  server.registerTool(
    "shotcut_get_speed_curve",
    {
      description: "Get the speed curve for a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("get_speed_curve", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
        }),
      ),
  );

  server.registerTool(
    "shotcut_remove_speed",
    {
      description: "Remove speed effects from a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("remove_speed", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
        }),
      ),
  );
}
