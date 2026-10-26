#pragma once

#include "CoreMinimal.h"
#include "AnimationDocumentTypes.h"

/** One uniformly sampled pose expressed as per-bone local-space transforms. */
struct AIANIMATIONSTUDIOCORE_API FArtiMotionSampledFrame
{
    float TimeSeconds = 0.0f;
    TMap<FName, FTransform> BoneTransforms;
};

/** Converts sparse tracks into uniformly sampled local-space poses. */
class AIANIMATIONSTUDIOCORE_API FArtiMotionDocumentSampler
{
public:
    static bool BuildSamples(
        const FAIAnimationDocument& Document,
        TArray<FArtiMotionSampledFrame>& OutFrames,
        TArray<FString>& OutErrors,
        const TArray<FName>& ValidBoneNames = TArray<FName>());
};
