# ArtiMotion for Unreal Engine

ArtiMotion is an Unreal Engine editor plugin for AI-assisted skeletal animation authoring. The initial target is Unreal Engine 5.8 and standard UE mannequin skeletons.

## Current milestone

The core JSON pipeline defines a canonical document format, parses and validates JSON, serializes documents back to JSON, and resamples sparse per-bone keys into uniform local-space pose frames. Native skeletal-mesh preview and `UAnimSequence` baking are the next stages and are not yet implemented.

## Pipeline

1. Import a JSON document with `FArtiMotionDocumentJson::ParseAndValidate`.
2. Optionally pass the target skeleton's bone names to reject unknown bone tracks.
3. Call `FArtiMotionDocumentSampler::BuildSamples` to create uniformly sampled local-space poses.
4. Preview those poses on the target skeleton, then bake to a native animation asset in the editor module (planned).

## Design principles

- The editable animation document is the source of truth; `UAnimSequence` is generated output.
- AI responses are structured data only. Never execute model-generated code.
- Time is in seconds, translations are in Unreal centimeters, and rotations are JSON quaternions `[x, y, z, w]`.
- All transforms are local to each bone's parent.
- Validate data before previewing or saving it.
- Keep provider integration separate from the animation data model.

## Installation

1. Copy this repository's plugin folder into your project's `Plugins` directory and name the folder `ArtiMotion`.
2. Regenerate project files if needed.
3. Build with the same Unreal Engine version as your project (initial target: UE 5.8).
4. Enable **ArtiMotion** in **Edit → Plugins**.
5. Open the **ArtiMotion** tab from the Window menu.

See `Docs/Architecture.md` for implementation details. This code has not yet been compiled against a local UE 5.8 installation.
