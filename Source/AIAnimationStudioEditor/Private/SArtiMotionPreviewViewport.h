#pragma once

#include "CoreMinimal.h"
#include "AnimationDocumentSampler.h"
#include "SEditorViewport.h"

class FAdvancedPreviewScene;
class FEditorViewportClient;
class USkeletalMesh;
class UPoseableMeshComponent;

/** Minimal editor viewport for previewing sampled ArtiMotion local-space poses. */
class SArtiMotionPreviewViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SArtiMotionPreviewViewport) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetPreviewMesh(USkeletalMesh* InMesh);
    USkeletalMesh* GetPreviewMesh() const;
    void SetSamples(const TArray<FArtiMotionSampledFrame>& InSamples, float InDurationSeconds);
    void SetPlaying(bool bInPlaying);
    void SetPlaybackTime(float InTimeSeconds);
    float GetPlaybackTime() const { return PlaybackTimeSeconds; }
    bool HasPreviewMesh() const;

protected:
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
    void ApplyCurrentFrame();

    TSharedPtr<FAdvancedPreviewScene> PreviewScene;
    TSharedPtr<FEditorViewportClient> PreviewViewportClient;
    UPoseableMeshComponent* PreviewComponent = nullptr;
    TArray<FArtiMotionSampledFrame> Samples;
    float DurationSeconds = 0.0f;
    float PlaybackTimeSeconds = 0.0f;
    bool bPlaying = false;
};
