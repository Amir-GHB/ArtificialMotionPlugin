# ArtiMotion architecture

## Current implementation

- `AIAnimationStudioRuntime`: reflected animation document, track, and keyframe types.
- `AIAnimationStudioCore`: document validation, JSON parse/serialize, and uniform pose sampling.
- `AIAnimationStudioEditor`: editor tab and future preview, AI-provider, and native asset baking integrations.

The internal module identifiers retain their original names for now; the plugin's user-facing name and descriptor are ArtiMotion.

## Canonical JSON schema

The example at `Content/Examples/SwordAttack.json` is the canonical schema. Top-level fields include `schemaVersion`, `documentId`, `revision`, `name`, `skeletonAssetPath`, `skeletonSignature`, `durationSeconds`, `sampleRate`, `rootMotionMode`, `rootBoneName`, and `tracks`.

Each track has a `boneName` and `keys`. Each key has `timeSeconds`, `translation` (three numbers), `rotation` (quaternion `[x, y, z, w]`), and `scale` (three numbers).

## Conventions and safeguards

- Time is in seconds; translations are in Unreal units (centimeters).
- Transforms are local to the parent bone.
- Sparse keys are interpolated against adjacent keys, with endpoints held constant.
- Translation and scale use linear interpolation; rotation uses quaternion slerp.
- Optional target bone names are checked during validation.
- Root-motion metadata does not itself extract root motion. That remains an asset-baking concern.
- Reject malformed or stale AI output; never execute generated code.

## Next milestones

1. Compile and test JSON import/export and the sampler in UE 5.8.
2. Add automated tests for malformed JSON, validation, quaternion order, and interpolation.
3. Add a preview component that applies sampled local transforms to a mannequin.
4. Add revision patches and targeted edits.
5. Bake a native `UAnimSequence` using the UE 5.8 animation data controller APIs.
6. Add Gemini integration with a secure key-management strategy.
