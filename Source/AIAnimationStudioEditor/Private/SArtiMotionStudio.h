#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AnimationDocumentTypes.h"
#include "AnimationDocumentSampler.h"

class SArtiMotionPreviewViewport;
class SEditableTextBox;
class STextBlock;

/** Main editor workflow: import JSON, resolve a preview mesh, and play/scrub sampled motion. */
class SArtiMotionStudio : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SArtiMotionStudio) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnLoadJsonClicked();
    FReply OnLoadPreviewMeshClicked();
    FReply OnPlayPauseClicked();
    FReply OnStopClicked();
    void OnMeshPathChanged(const FText& NewText);
    void OnTimelineChanged(float NewValue);
    void RefreshSamples();
    bool TryLoadPreviewMesh(const FString& ObjectPath);
    TArray<FName> GetPreviewBoneNames() const;
    FText GetStatusText() const;
    FText GetPlayButtonText() const;

    FAIAnimationDocument Document;
    TArray<FArtiMotionSampledFrame> Samples;
    TSharedPtr<SArtiMotionPreviewViewport> PreviewViewport;
    TSharedPtr<SEditableTextBox> MeshPathTextBox;
    TSharedPtr<STextBlock> StatusText;
    FString PreviewMeshPath;
    FString StatusMessage = TEXT("Load an ArtiMotion JSON document to begin.");
    bool bDocumentLoaded = false;
    bool bPlaying = false;
};
