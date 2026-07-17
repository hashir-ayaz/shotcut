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

export function registerKeyframeTools(
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
    "shotcut_list_animatable_properties",
    {
      description: "List animatable CapCut-style properties for a clip",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("list_animatable_properties", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
        }),
      ),
  );

  server.registerTool(
    "shotcut_add_keyframe",
    {
      description: "Add a keyframe on a clip property",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        propertyId: z.string(),
        frame: z.number().int(),
        value: z.number(),
        interpolation: z.string().optional(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("add_keyframe", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
          propertyId: args.propertyId,
          frame: args.frame,
          value: args.value,
          interpolation: args.interpolation ?? "linear",
        }),
      ),
  );

  server.registerTool(
    "shotcut_remove_keyframe",
    {
      description: "Remove a keyframe from a clip property",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        propertyId: z.string(),
        frame: z.number().int(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("remove_keyframe", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
          propertyId: args.propertyId,
          frame: args.frame,
        }),
      ),
  );

  server.registerTool(
    "shotcut_list_keyframes",
    {
      description: "List keyframes for a clip property",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        propertyId: z.string(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("list_keyframes", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
          propertyId: args.propertyId,
        }),
      ),
  );

  server.registerTool(
    "shotcut_set_clip_property",
    {
      description: "Set a clip property value at the playhead",
      inputSchema: {
        trackIndex: z.number().int(),
        clipIndex: z.number().int(),
        propertyId: z.string(),
        value: z.number(),
      },
    },
    async (args) =>
      formatResult(
        await client.request("set_clip_property", {
          trackIndex: args.trackIndex,
          clipIndex: args.clipIndex,
          propertyId: args.propertyId,
          value: args.value,
        }),
      ),
  );
}
