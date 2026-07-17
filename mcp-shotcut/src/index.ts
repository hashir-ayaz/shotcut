import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { z } from "zod";

import { ShotcutClient } from "./shotcutClient.js";
import { registerCaptionTools } from "./tools/captions.js";
import { registerKeyframeTools } from "./tools/keyframes.js";
import { registerTransitionTools } from "./tools/transitions.js";

const client = new ShotcutClient();

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

const server = new McpServer({
  name: "shotcut",
  version: "0.2.0",
});

server.registerTool(
  "shotcut_ping",
  {
    description: "Ping the running Shotcut EditorService",
    inputSchema: {},
  },
  async () => formatResult(await client.request("ping")),
);

server.registerTool(
  "shotcut_get_state",
  {
    description: "Get current Shotcut project/timeline state",
    inputSchema: {},
  },
  async () => formatResult(await client.request("get_state")),
);

server.registerTool(
  "shotcut_open_project",
  {
    description: "Open a Shotcut project (.mlt) or media file",
    inputSchema: {
      path: z.string().describe("Absolute path to the project or media file"),
    },
  },
  async ({ path }) => formatResult(await client.request("open_project", { path })),
);

server.registerTool(
  "shotcut_save_project",
  {
    description: "Save the current Shotcut project",
    inputSchema: {
      path: z
        .string()
        .optional()
        .describe("Optional absolute path; uses current project path when omitted"),
    },
  },
  async ({ path }) => formatResult(await client.request("save_project", path ? { path } : {})),
);

server.registerTool(
  "shotcut_add_clip",
  {
    description: "Add a media clip to the timeline via EditorService",
    inputSchema: {
      path: z.string().describe("Absolute path to the media file"),
      trackIndex: z
        .number()
        .int()
        .optional()
        .describe("Timeline track index; defaults to current track"),
      position: z
        .number()
        .int()
        .optional()
        .describe("Timeline position in frames; defaults to playhead"),
    },
  },
  async ({ path, trackIndex, position }) => {
    const params: Record<string, unknown> = { path };
    if (trackIndex !== undefined) params.trackIndex = trackIndex;
    if (position !== undefined) params.position = position;
    return formatResult(await client.request("add_clip", params));
  },
);

registerTransitionTools(server, client);
registerKeyframeTools(server, client);
registerCaptionTools(server, client);

const transport = new StdioServerTransport();
await server.connect(transport);
console.error("mcp-shotcut running on stdio");
