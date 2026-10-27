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
    // PoseableMeshComponent needs its tick enabled to process dirty bone transforms.
    PreviewComponent->SetComponentTickEnabled(true);
    PreviewScene->AddComponent(PreviewComponent, FTransform::Identity);
    UE_LOG(LogTemp, Display, TEXT("[ArtiMotion][Viewport] Constructed poseable preview component."));
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

    PreviewComponent->SetSkinnedAssetAndUpdate(InMesh, true);
    UE_LOG(LogTemp, Display, TEXT("[ArtiMotion][Viewport] SetPreviewMesh: mesh=%s bones=%d"),
        InMesh ? *InMesh->GetName() : TEXT("<null>"),
        InMesh ? InMesh->GetRefSkeleton().GetNum() : 0);
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
    return PreviewComponent ? Cast<USkeletalMesh>(PreviewComponent->GetSkinnedAsset()) : nullptr;
}

void SArtiMotionPreviewViewport::SetSamples(
    const TArray<FArtiMotionSampledFrame>& InSamples,
    float InDurationSeconds)
{
    Samples = InSamples;
    DurationSeconds = FMath::Max(0.0f, InDurationSeconds);
    PlaybackTimeSeconds = 0.0f;
    UE_LOG(LogTemp, Display, TEXT("[ArtiMotion][Samples] SetSamples: frames=%d duration=%.3f mesh=%s"),
        Samples.Num(), DurationSeconds, GetPreviewMesh() ? *GetPreviewMesh()->GetName() : TEXT("<no mesh>"));
    if (Samples.Num() > 0)
    {
        UE_LOG(LogTemp, Display, TEXT("[ArtiMotion][Samples] First frame: time=%.3f boneTransforms=%d"),
            Samples[0].TimeSeconds, Samples[0].BoneTransforms.Num());
        for (const TPair<FName, FTransform>& Pair : Samples[0].BoneTransforms)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[ArtiMotion][Samples] First-frame bone=%s rot=(%.3f, %.3f, %.3f, %.3f)"),
                *Pair.Key.ToString(), Pair.Value.GetRotation().X, Pair.Value.GetRotation().Y,
                Pair.Value.GetRotation().Z, Pair.Value.GetRotation().W);
        }
    }
    ApplyCurrentFrame();
}

void SArtiMotionPreviewViewport::SetPlaying(bool bInPlaying)
{
    bPlaying = bInPlaying && Samples.Num() > 1 && DurationSeconds > 0.0f;
    UE_LOG(LogTemp, Display, TEXT("[ArtiMotion][Playback] SetPlaying requested=%s actual=%s frames=%d duration=%.3f mesh=%s"),
        bInPlaying ? TEXT("true") : TEXT("false"), bPlaying ? TEXT("true") : TEXT("false"),
        Samples.Num(), DurationSeconds, GetPreviewMesh() ? *GetPreviewMesh()->GetName() : TEXT("<no mesh>"));
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
        DiagnosticLogAccumulator += InDeltaTime;
        if (DiagnosticLogAccumulator >= 1.0f)
        {
            DiagnosticLogAccumulator = 0.0f;
            UE_LOG(LogTemp, Display, TEXT("[ArtiMotion][Playback] Tick advancing: time=%.3f delta=%.4f frames=%d"),
                PlaybackTimeSeconds, InDeltaTime, Samples.Num());
        }
        ApplyCurrentFrame();
    }
}

void SArtiMotionPreviewViewport::ApplyCurrentFrame()
{
    if (!PreviewComponent || !GetPreviewMesh() || Samples.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArtiMotion][Pose] ApplyCurrentFrame aborted: component=%s mesh=%s samples=%d"),
            PreviewComponent ? TEXT("valid") : TEXT("null"), GetPreviewMesh() ? TEXT("valid") : TEXT("null"), Samples.Num());
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

        const int32 BoneIndex = GetPreviewMesh()->GetRefSkeleton().FindBoneIndex(BoneTransform.Key);

        // ArtiMotion stores local-to-parent transforms. In UE 5.8, EBoneSpaces
        // only exposes world and component space, so write directly to the
        // poseable component's local BoneSpaceTransforms array.
        const bool bValidBoneTransformIndex = BoneIndex != INDEX_NONE && PreviewComponent->BoneSpaceTransforms.IsValidIndex(BoneIndex);
        const bool bLogThisPose = PlaybackTimeSeconds <= UE_SMALL_NUMBER ||
            FMath::FloorToInt(PlaybackTimeSeconds) != FMath::FloorToInt(LastPoseDiagnosticTime);
        if (bLogThisPose)
        {
            UE_LOG(LogTemp, Display, TEXT("[ArtiMotion][Pose] t=%.3f bone=%s index=%d transformSlotValid=%s requestedRot=(%.3f, %.3f, %.3f, %.3f)"),
                PlaybackTimeSeconds, *BoneTransform.Key.ToString(), BoneIndex,
                bValidBoneTransformIndex ? TEXT("true") : TEXT("false"),
                BlendedTransform.GetRotation().X, BlendedTransform.GetRotation().Y,
                BlendedTransform.GetRotation().Z, BlendedTransform.GetRotation().W);
        }
        if (bValidBoneTransformIndex)
        {
            PreviewComponent->BoneSpaceTransforms[BoneIndex] = BlendedTransform;
        }
    }

    PreviewComponent->FillComponentSpaceTransforms();
    PreviewComponent->MarkRefreshTransformDirty();
    PreviewComponent->MarkRenderDynamicDataDirty();
    LastPoseDiagnosticTime = PlaybackTimeSeconds;
}
