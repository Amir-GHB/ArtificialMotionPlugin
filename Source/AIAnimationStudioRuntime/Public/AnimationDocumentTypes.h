#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "AnimationDocumentTypes.generated.h"

UENUM(BlueprintType)
enum class EAIAnimationRootMotionMode : uint8
{
    InPlace UMETA(DisplayName = "In Place"),
    ExtractFromRoot UMETA(DisplayName = "Extract From Root"),
    PreserveRootTrack UMETA(DisplayName = "Preserve Root Track")
};

/** One local-space transform key. JSON quaternion order is [x, y, z, w]. */
USTRUCT(BlueprintType)
struct AIANIMATIONSTUDIORUNTIME_API FAIAnimationKeyframe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    float TimeSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FVector Translation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FQuat Rotation = FQuat::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FVector Scale = FVector::OneVector;
};

/** Sparse animation keys for one named skeleton bone. */
USTRUCT(BlueprintType)
struct AIANIMATIONSTUDIORUNTIME_API FAIAnimationBoneTrack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FName BoneName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    TArray<FAIAnimationKeyframe> Keys;
};

/**
 * Editable source document. AI edits apply to this data; a separate builder
 * later bakes it into a native UAnimSequence asset.
 */
USTRUCT(BlueprintType)
struct AIANIMATIONSTUDIORUNTIME_API FAIAnimationDocument
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FString SchemaVersion = TEXT("1.0");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FGuid DocumentId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    int32 Revision = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FString Name = TEXT("NewAnimation");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FSoftObjectPath SkeletonAssetPath;

    /** Optional signature derived from the target skeleton and reference pose. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FString SkeletonSignature;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation", meta = (ClampMin = "0.001"))
    float DurationSeconds = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation", meta = (ClampMin = "1"))
    int32 SampleRate = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    EAIAnimationRootMotionMode RootMotionMode = EAIAnimationRootMotionMode::InPlace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    FName RootBoneName = TEXT("root");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Animation")
    TArray<FAIAnimationBoneTrack> Tracks;
};