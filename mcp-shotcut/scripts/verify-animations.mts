import { ShotcutClient } from "../src/shotcutClient.js";

const client = new ShotcutClient();

async function main() {
  console.log("ping", JSON.stringify(await client.request("ping")));

  console.log(
    "open",
    JSON.stringify(
      await client.request("open_project", {
        path: "/Users/hashirayaz/development/shotcut/testdata/phase2-verify.mlt",
      }),
    ),
  );

  console.log(
    "fade in",
    JSON.stringify(
      await client.request("apply_clip_animation", {
        trackIndex: 0,
        clipIndex: 0,
        presetId: "fade",
        durationFrames: 45,
        mode: "in",
      }),
    ),
  );

  console.log(
    "slide out",
    JSON.stringify(
      await client.request("apply_clip_animation", {
        trackIndex: 0,
        clipIndex: 0,
        presetId: "slide_left",
        durationFrames: 30,
        mode: "out",
      }),
    ),
  );

  console.log(
    "zoom in clip2",
    JSON.stringify(
      await client.request("apply_clip_animation", {
        trackIndex: 0,
        clipIndex: 2,
        presetId: "zoom_in",
        durationFrames: 30,
        mode: "in",
      }),
    ),
  );

  console.log(
    "opacity",
    JSON.stringify(
      await client.request("list_keyframes", {
        trackIndex: 0,
        clipIndex: 0,
        propertyId: "opacity",
      }),
      null,
      2,
    ),
  );
  console.log(
    "position.x",
    JSON.stringify(
      await client.request("list_keyframes", {
        trackIndex: 0,
        clipIndex: 0,
        propertyId: "position.x",
      }),
      null,
      2,
    ),
  );
  console.log(
    "scale clip2",
    JSON.stringify(
      await client.request("list_keyframes", {
        trackIndex: 0,
        clipIndex: 2,
        propertyId: "scale",
      }),
      null,
      2,
    ),
  );
}

main()
  .catch((error) => {
    console.error(error);
    process.exit(1);
  })
  .finally(() => client.close());
