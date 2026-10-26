# Architecture and implementation notes

## First milestone

The first implementation is intentionally a scaffold:
- Unreal plugin descriptor and three modules.
- Shared reflected animation document, track, and keyframe structs.
- Stateless document validation.
- Dockable editor tab proving that the editor module loads.

The tab is a placeholder. It does not yet preview a skeletal mesh, call Gemini, or bake an animation.

## Module boundaries

- AIAnimationStudioRuntime owns shared reflected data structures.
- AIAnimationStudioCore owns document validation and later interpolation, patch application, skeleton mapping, and transform conversion.
- AIAnimationStudioEditor owns Slate UI, preview scenes, AI provider adapters, editor settings, revision management, and native asset generation.

## Canonical document conventions

- Time is expressed in seconds.
- Translation uses Unreal units (centimeters) after input conversion.
- Rotations are quaternions. JSON uses [x, y, z, w]; conversion to and from FQuat must be explicit.
- Tracks are local-space transforms relative to each bone's parent.
- Tracks may be sparse. A preview/bake layer must resolve them against a defined starting pose and interpolate missing samples.
- AI revisions should use patches tied to a base revision; stale patches must be rejected.
- Never execute model-generated code. Accept validated structured data only.

## Planned milestones

1. Plugin/module scaffold.
2. JSON read/write and validator tests.
3. Mannequin selection, skeleton profile, and document-driven preview.
4. Gemini provider and structured keyframe generation.
5. Revision patches, undo/redo, and targeted edits.
6. UAnimSequence baking through the animation data model/controller.
7. Timeline, bone gizmos, revision comparison, and quality constraints.

## Important implementation cautions

- Verify bone names and the skeleton/reference-pose signature against the actual target asset. Do not assume one mannequin asset path or bone naming scheme is universal.
- Keep previewing separate from asset baking; do not create a Content Browser asset for every intermediate AI response.
- Validate finite values, key ordering, duration bounds, quaternion magnitude, and bone existence before applying AI output.
- Root-motion extraction is an asset-generation concern and is not implemented by document metadata alone.
- Animation data model/controller APIs are engine-version-sensitive. Confirm exact signatures in the target UE 5.8 headers before implementing the builder.
