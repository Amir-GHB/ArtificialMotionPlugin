#include "SArtiMotionPreviewViewport.h"

#include "AdvancedPreviewScene.h"
#include "Components/PoseableMeshComponent.h"
#include "EditorViewportClient.h"
#include "Engine/SkeletalMesh.h"

void SArtiMotionPreviewViewport::Construct(const FArguments& InArgs)
{
    PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
    SEditorViewport::Construct(SEditorViewport::FArguments());

    PreviewComponent = NewObject<UPoseableMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
    PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewComponent->SetGenerateOverlapEvents(false);
    PreviewComponent->SetComponentTickEnabled(false);
    PreviewComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    PreviewScene->AddComponent(PreviewComponent, FTransform::Identity);
}

TSharedRef<FEditorViewportClient> SArtiMotionPreviewViewport::MakeEditorViewportClient()
{
    PreviewViewportClient = MakeShared<FEditorViewportClient>(nullptr, PreviewScene.Get(), SharedThis(this));
    PreviewViewportClient->SetViewLocation(FVector(260.0f, 260.0f, 110.0f));
    PreviewViewportClient->SetViewRotation(FRotator(-12.0f, -135.0f, 0.0f));
    PreviewViewportClient->SetRealtime(true);
    return PreviewViewportClient.ToSharedRef();
}

void SArtiMotionPreviewViewport::SetPreviewMesh(USkeletalMesh* InMesh)
{
    if (!PreviewComponent)
    {
        return;
    }

    PreviewComponent->SetSkeletalMesh(InMesh);
    Samples.Reset();
    PlaybackTimeSeconds = 0.0f;
    bPlaying = false;
    ApplyCurrentFrame();

    if (PreviewViewportClient.IsValid())
    {
        PreviewViewportClient->Invalidate();
    }
}

USkeletalMesh* SArtiMotionPreviewViewport::GetPreviewMesh() const
{
    return PreviewComponent ? PreviewComponent->GetSkeletalMeshAsset() : nullptr;
}

void SArtiMotionPreviewViewport::SetSamples(
    const TArray<FArtiMotionSampledFrame>& InSamples,
    float InDurationSeconds)
{
    Samples = InSamples;
    DurationSeconds = FMath::Max(0.0f, InDurationSeconds);
    PlaybackTimeSeconds = 0.0f;
    ApplyCurrentFrame();
}

void SArtiMotionPreviewViewport::SetPlaying(bool bInPlaying)
{
    bPlaying = bInPlaying && Samples.Num() > 1 && DurationSeconds > 0.0f;
}

void SArtiMotionPreviewViewport::SetPlaybackTime(float InTimeSeconds)
{
    PlaybackTimeSeconds = FMath::Clamp(InTimeSeconds, 0.0f, DurationSeconds);
    ApplyCurrentFrame();
}

bool SArtiMotionPreviewViewport::HasPreviewMesh() const
{
    return GetPreviewMesh() != nullptr;
}

void SArtiMotionPreviewViewport::Tick(
    const FGeometry& AllottedGeometry,
    double InCurrentTime,
    float InDeltaTime)
{
    SEditorViewport::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    if (bPlaying && DurationSeconds > UE_SMALL_NUMBER)
    {
        PlaybackTimeSeconds = FMath::Fmod(PlaybackTimeSeconds + InDeltaTime, DurationSeconds);
        ApplyCurrentFrame();
    }
}

void SArtiMotionPreviewViewport::ApplyCurrentFrame()
{
    if (!PreviewComponent || !PreviewComponent->GetSkeletalMeshAsset() || Samples.IsEmpty())
    {
        return;
    }

    int32 LeftIndex = 0;
    while (LeftIndex + 1 < Samples.Num() && Samples[LeftIndex + 1].TimeSeconds < PlaybackTimeSeconds)
    {
        ++LeftIndex;
    }

    const int32 RightIndex = FMath::Min(LeftIndex + 1, Samples.Num() - 1);
    const FArtiMotionSampledFrame& LeftFrame = Samples[LeftIndex];
    const FArtiMotionSampledFrame& RightFrame = Samples[RightIndex];
    const float Span = RightFrame.TimeSeconds - LeftFrame.TimeSeconds;
    const float Alpha = Span > UE_SMALL_NUMBER
        ? FMath::Clamp((PlaybackTimeSeconds - LeftFrame.TimeSeconds) / Span, 0.0f, 1.0f)
        : 0.0f;

    for (const TPair<FName, FTransform>& BoneTransform : LeftFrame.BoneTransforms)
    {
        const FTransform* RightTransform = RightFrame.BoneTransforms.Find(BoneTransform.Key);
        const FTransform& A = BoneTransform.Value;
        const FTransform& B = RightTransform ? *RightTransform : A;

        const FTransform BlendedTransform(
            FQuat::Slerp(A.GetRotation(), B.GetRotation(), Alpha).GetNormalized(),
            FMath::Lerp(A.GetLocation(), B.GetLocation(), Alpha),
            FMath::Lerp(A.GetScale3D(), B.GetScale3D(), Alpha));

        PreviewComponent->SetBoneTransformByName(
            BoneTransform.Key,
            BlendedTransform,
            EBoneSpaces::LocalSpace);
    }

    PreviewComponent->MarkRenderDynamicDataDirty();
}
