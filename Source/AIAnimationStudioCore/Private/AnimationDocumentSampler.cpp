#include "AnimationDocumentSampler.h"
#include "AnimationDocumentValidator.h"

namespace
{
    FTransform EvaluateTrack(const FAIAnimationBoneTrack& Track, float TimeSeconds)
    {
        if (Track.Keys.Num() == 1 || TimeSeconds <= Track.Keys[0].TimeSeconds)
        {
            const FAIAnimationKeyframe& Key = Track.Keys[0];
            return FTransform(Key.Rotation, Key.Translation, Key.Scale);
        }

        const FAIAnimationKeyframe& LastKey = Track.Keys.Last();
        if (TimeSeconds >= LastKey.TimeSeconds)
        {
            return FTransform(LastKey.Rotation, LastKey.Translation, LastKey.Scale);
        }

        for (int32 Index = 1; Index < Track.Keys.Num(); ++Index)
        {
            const FAIAnimationKeyframe& Right = Track.Keys[Index];
            if (TimeSeconds <= Right.TimeSeconds)
            {
                const FAIAnimationKeyframe& Left = Track.Keys[Index - 1];
                const float Span = Right.TimeSeconds - Left.TimeSeconds;
                const float Alpha = Span > UE_SMALL_NUMBER
                    ? FMath::Clamp((TimeSeconds - Left.TimeSeconds) / Span, 0.0f, 1.0f)
                    : 1.0f;
                return FTransform(
                    FQuat::Slerp(Left.Rotation, Right.Rotation, Alpha).GetNormalized(),
                    FMath::Lerp(Left.Translation, Right.Translation, Alpha),
                    FMath::Lerp(Left.Scale, Right.Scale, Alpha));
            }
        }
        return FTransform::Identity;
    }
}

bool FArtiMotionDocumentSampler::BuildSamples(
    const FAIAnimationDocument& Document,
    TArray<FArtiMotionSampledFrame>& OutFrames,
    TArray<FString>& OutErrors,
    const TArray<FName>& ValidBoneNames)
{
    OutFrames.Reset();
    if (!FAIAnimationDocumentValidator::Validate(Document, OutErrors, ValidBoneNames))
    {
        return false;
    }

    const int32 LastFrameIndex = FMath::Max(1, FMath::CeilToInt(Document.DurationSeconds * Document.SampleRate));
    OutFrames.Reserve(LastFrameIndex + 1);
    for (int32 FrameIndex = 0; FrameIndex <= LastFrameIndex; ++FrameIndex)
    {
        FArtiMotionSampledFrame& Frame = OutFrames.AddDefaulted_GetRef();
        Frame.TimeSeconds = FMath::Min(
            static_cast<float>(FrameIndex) / static_cast<float>(Document.SampleRate),
            Document.DurationSeconds);

        for (const FAIAnimationBoneTrack& Track : Document.Tracks)
        {
            if (!Track.Keys.IsEmpty())
            {
                Frame.BoneTransforms.Add(Track.BoneName, EvaluateTrack(Track, Frame.TimeSeconds));
            }
        }
    }
    return true;
}
