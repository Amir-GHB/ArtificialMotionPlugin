# AI Animation Studio for Unreal Engine

An Unreal Editor plugin for AI-assisted skeletal animation authoring. The initial target is Unreal Engine 5.8 and the standard UE mannequin skeleton.

## Current status

**Phase 1: repository and plugin foundation.** This repository is being built incrementally. The first milestone is a loadable editor plugin and a versioned animation document model, followed by validation, preview, Gemini integration, revision patches, and native `UAnimSequence` baking.

## Design principles

- The editable animation document is the source of truth; `UAnimSequence` is a generated output.
- AI responses are structured data only. Never execute model-generated code.
- Bone transforms use local space and quaternion rotations in the document format.
- Validate all AI-produced data before previewing or saving it.
- Keep AI provider integration separate from the animation data model.
- Use revision IDs so stale AI responses cannot overwrite newer edits.

## Planned modules

- `AIAnimationStudioRuntime`: shared reflected document and keyframe types.
- `AIAnimationStudioCore`: document validation and animation-data operations.
- `AIAnimationStudioEditor`: dockable editor tab and editor-only integrations.

## Installation

1. Copy the `AIAnimationStudio` directory into your project's `Plugins` directory.
2. Regenerate project files if needed.
3. Build the project using the same Unreal Engine version as the project (initial target: UE 5.8).
4. Enable **AI Animation Studio** in **Edit → Plugins**.
5. Open the **AI Animation Studio** tab from the Window menu after compiling.

The plugin is an early scaffold; AI generation, animation viewport playback, and native asset baking are not implemented yet.

## Animation document conventions

- Time is in seconds.
- Translation is in centimeters after conversion to Unreal units.
- Rotation is a quaternion in JSON order `[x, y, z, w]`.
- Bone transforms are local to the parent bone.
- The target skeleton must be validated against its actual bone hierarchy and reference pose.

See `Docs/Architecture.md` for the initial implementation plan.