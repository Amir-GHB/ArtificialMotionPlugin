#include "AnimationDocumentValidator.h"

bool FAIAnimationDocumentValidator::Validate(
    const FAIAnimationDocument& Document,
    TArray<FString>& OutErrors,
    const TArray<FName>& ValidBoneNames)
{
    OutErrors.Reset();

    if (Document.SchemaVersion != TEXT("1.0"))
    {
        OutErrors.Add(FString::Printf(TEXT("Unsupported schema version '%s'."), *Document.SchemaVersion));
    }
    if (Document.Name.TrimStartAndEnd().IsEmpty())
    {
        OutErrors.Add(TEXT("Animation name must not be empty."));
    }
    if (!FMath::IsFinite(Document.DurationSeconds) || Document.DurationSeconds <= 0.0f)
    {
        OutErrors.Add(TEXT("DurationSeconds must be finite and greater than zero."));
    }
    if (Document.SampleRate < 1 || Document.SampleRate > 240)
    {
        OutErrors.Add(TEXT("SampleRate must be between 1 and 240 FPS."));
    }

    TSet<FName> SeenBones;
    TSet<FName> AllowedBones;
    for (const FName& BoneName : ValidBoneNames)
    {
        AllowedBones.Add(BoneName);
    }

    for (const FAIAnimationBoneTrack& Track : Document.Tracks)
    {
        if (Track.BoneName.IsNone())
        {
            OutErrors.Add(TEXT("A track has no bone name."));
            continue;
        }
        if (SeenBones.Contains(Track.BoneName))
        {
            OutErrors.Add(FString::Printf(TEXT("Duplicate track for bone '%s'."), *Track.BoneName.ToString()));
        }
        SeenBones.Add(Track.BoneName);

        if (ValidBoneNames.Num() > 0 && !AllowedBones.Contains(Track.BoneName))
        {
            OutErrors.Add(FString::Printf(TEXT("Bone '%s' is not present in the target skeleton."), *Track.BoneName.ToString()));
        }

        float PreviousTime = -1.0f;
        for (int32 KeyIndex = 0; KeyIndex < Track.Keys.Num(); ++KeyIndex)
        {
            const FAIAnimationKeyframe& Key = Track.Keys[KeyIndex];

            if (!FMath::IsFinite(Key.TimeSeconds) || Key.TimeSeconds < 0.0f ||
                Key.TimeSeconds > Document.DurationSeconds)
            {
                OutErrors.Add(FString::Printf(
                    TEXT("Track '%s' key %d has a time outside the animation duration."),
                    *Track.BoneName.ToString(), KeyIndex));
            }
            if (KeyIndex > 0 && Key.TimeSeconds < PreviousTime)
            {
                OutErrors.Add(FString::Printf(
                    TEXT("Track '%s' keys must be ordered by time."),
                    *Track.BoneName.ToString()));
            }
            PreviousTime = Key.TimeSeconds;

            const FVector& T = Key.Translation;
            const FVector& S = Key.Scale;
            if (T.ContainsNaN() || S.ContainsNaN() ||
                !FMath::IsFinite(T.X) || !FMath::IsFinite(T.Y) || !FMath::IsFinite(T.Z) ||
                !FMath::IsFinite(S.X) || !FMath::IsFinite(S.Y) || !FMath::IsFinite(S.Z))
            {
                OutErrors.Add(FString::Printf(
                    TEXT("Track '%s' key %d contains a non-finite translation or scale."),
                    *Track.BoneName.ToString(), KeyIndex));
            }

            const FQuat& Q = Key.Rotation;
            if (Q.ContainsNaN() ||
                !FMath::IsFinite(Q.X) || !FMath::IsFinite(Q.Y) ||
                !FMath::IsFinite(Q.Z) || !FMath::IsFinite(Q.W) ||
                Q.SizeSquared() < UE_SMALL_NUMBER)
            {
                OutErrors.Add(FString::Printf(
                    TEXT("Track '%s' key %d contains an invalid quaternion."),
                    *Track.BoneName.ToString(), KeyIndex));
            }
            else if (!FMath::IsNearlyEqual(Q.SizeSquared(), 1.0f, 0.05f))
            {
                OutErrors.Add(FString::Printf(
                    TEXT("Track '%s' key %d has a non-normalized quaternion."),
                    *Track.BoneName.ToString(), KeyIndex));
            }
        }
    }

    return OutErrors.IsEmpty();
}