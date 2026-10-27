#include "SArtiMotionStudio.h"

#include "AnimationDocumentJson.h"
#include "AnimationDocumentSampler.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SArtiMotionPreviewViewport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SArtiMotionStudio"

void SArtiMotionStudio::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(8.0f)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0.0f, 0.0f, 6.0f, 0.0f)
            [
                SNew(SButton)
                .Text(LOCTEXT("LoadJson", "Load JSON"))
                .OnClicked(this, &SArtiMotionStudio::OnLoadJsonClicked)
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0.0f, 0.0f, 6.0f, 0.0f)
            [
                SNew(SButton)
                .Text(this, &SArtiMotionStudio::GetPlayButtonText)
                .OnClicked(this, &SArtiMotionStudio::OnPlayPauseClicked)
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("Stop", "Stop"))
                .OnClicked(this, &SArtiMotionStudio::OnStopClicked)
            ]
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(8.0f, 0.0f, 8.0f, 6.0f)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0.0f, 0.0f, 6.0f, 0.0f)
            [
                SAssignNew(MeshPathTextBox, SEditableTextBox)
                .HintText(LOCTEXT("MeshPathHint", "Skeletal Mesh object path, e.g. /Game/Characters/SK_Mannequin.SK_Mannequin"))
                .Text_Lambda([this]() { return FText::FromString(PreviewMeshPath); })
                .OnTextChanged(this, &SArtiMotionStudio::OnMeshPathChanged)
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("LoadMesh", "Load Preview Mesh"))
                .OnClicked(this, &SArtiMotionStudio::OnLoadPreviewMeshClicked)
            ]
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(8.0f, 0.0f, 8.0f, 8.0f)
        [
            SNew(SSlider)
            .Value_Lambda([this]()
            {
                return Document.DurationSeconds > UE_SMALL_NUMBER && PreviewViewport.IsValid()
                    ? PreviewViewport->GetPlaybackTime() / Document.DurationSeconds
                    : 0.0f;
            })
            .OnValueChanged(this, &SArtiMotionStudio::OnTimelineChanged)
        ]

        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(8.0f)
        [
            SAssignNew(PreviewViewport, SArtiMotionPreviewViewport)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(8.0f)
        [
            SNew(SBorder)
            .Padding(8.0f)
            [
                SAssignNew(StatusText, STextBlock)
                .AutoWrapText(true)
                .Text(this, &SArtiMotionStudio::GetStatusText)
            ]
        ]
    ];
}

FReply SArtiMotionStudio::OnLoadJsonClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        StatusMessage = TEXT("The desktop file dialog is unavailable on this platform.");
        return FReply::Handled();
    }

    TArray<FString> SelectedFiles;
    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
    const bool bPicked = DesktopPlatform->OpenFileDialog(
        ParentWindowHandle,
        TEXT("Load ArtiMotion animation JSON"),
        FPaths::ProjectDir(),
        FString(),
        TEXT("JSON files (*.json)|*.json"),
        EFileDialogFlags::None,
        SelectedFiles);

    if (!bPicked || SelectedFiles.IsEmpty())
    {
        return FReply::Handled();
    }

    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *SelectedFiles[0]))
    {
        StatusMessage = FString::Printf(TEXT("Could not read file: %s"), *SelectedFiles[0]);
        return FReply::Handled();
    }

    TArray<FString> Errors;
    FAIAnimationDocument ParsedDocument;
    if (!FArtiMotionDocumentJson::ParseAndValidate(JsonString, ParsedDocument, Errors))
    {
        StatusMessage = TEXT("JSON import failed:\n") + FString::Join(Errors, TEXT("\n"));
        return FReply::Handled();
    }

    Document = MoveTemp(ParsedDocument);
    bDocumentLoaded = true;
    bPlaying = false;
    PreviewMeshPath = Document.SkeletonAssetPath.ToString();
    if (MeshPathTextBox.IsValid())
    {
        MeshPathTextBox->SetText(FText::FromString(PreviewMeshPath));
    }

    if (!PreviewMeshPath.IsEmpty())
    {
        TryLoadPreviewMesh(PreviewMeshPath);
    }
    RefreshSamples();

    StatusMessage = FString::Printf(
        TEXT("Loaded '%s' | %.2f seconds | %d Hz | %d bone tracks | %d sampled frames"),
        *Document.Name,
        Document.DurationSeconds,
        Document.SampleRate,
        Document.Tracks.Num(),
        Samples.Num());

    if (!PreviewViewport->HasPreviewMesh())
    {
        StatusMessage += TEXT("\nNo preview mesh loaded. Enter a Skeletal Mesh object path above and click Load Preview Mesh.");
    }
    return FReply::Handled();
}

FReply SArtiMotionStudio::OnLoadPreviewMeshClicked()
{
    if (TryLoadPreviewMesh(PreviewMeshPath))
    {
        RefreshSamples();
        StatusMessage = FString::Printf(
            TEXT("Preview mesh loaded: %s | %d animation samples"),
            PreviewViewport->GetPreviewMesh() ? *PreviewViewport->GetPreviewMesh()->GetName() : TEXT("Unknown"),
            Samples.Num());
    }
    return FReply::Handled();
}

bool SArtiMotionStudio::TryLoadPreviewMesh(const FString& ObjectPath)
{
    if (ObjectPath.TrimStartAndEnd().IsEmpty())
    {
        StatusMessage = TEXT("Enter a Skeletal Mesh object path. Use Copy Reference in the Content Browser.");
        return false;
    }

    const FSoftObjectPath SoftPath(ObjectPath.TrimStartAndEnd());
    UObject* LoadedObject = SoftPath.TryLoad();
    USkeletalMesh* Mesh = Cast<USkeletalMesh>(LoadedObject);
    if (!Mesh)
    {
        StatusMessage = FString::Printf(
            TEXT("Could not load a USkeletalMesh from '%s'. Use the Skeletal Mesh asset's object path, not a Skeleton asset path."),
            *ObjectPath);
        return false;
    }

    PreviewViewport->SetPreviewMesh(Mesh);
    PreviewMeshPath = SoftPath.ToString();
    if (bDocumentLoaded)
    {
        Document.SkeletonAssetPath = SoftPath;
    }
    return true;
}

TArray<FName> SArtiMotionStudio::GetPreviewBoneNames() const
{
    TArray<FName> Names;
    const USkeletalMesh* Mesh = PreviewViewport.IsValid() ? PreviewViewport->GetPreviewMesh() : nullptr;
    if (!Mesh)
    {
        return Names;
    }

    const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
    Names.Reserve(RefSkeleton.GetNum());
    for (int32 Index = 0; Index < RefSkeleton.GetNum(); ++Index)
    {
        Names.Add(RefSkeleton.GetBoneName(Index));
    }
    return Names;
}

void SArtiMotionStudio::RefreshSamples()
{
    Samples.Reset();
    if (!bDocumentLoaded)
    {
        return;
    }

    TArray<FString> Errors;
    const TArray<FName> BoneNames = GetPreviewBoneNames();
    if (!FArtiMotionDocumentSampler::BuildSamples(Document, Samples, Errors, BoneNames))
    {
        StatusMessage = TEXT("Animation validation failed:\n") + FString::Join(Errors, TEXT("\n"));
        return;
    }

    if (PreviewViewport.IsValid())
    {
        PreviewViewport->SetSamples(Samples, Document.DurationSeconds);
        PreviewViewport->SetPlaying(bPlaying);
    }
}

void SArtiMotionStudio::OnMeshPathChanged(const FText& NewText)
{
    PreviewMeshPath = NewText.ToString();
}

FReply SArtiMotionStudio::OnPlayPauseClicked()
{
    if (!bDocumentLoaded || Samples.Num() < 2 || !PreviewViewport->HasPreviewMesh())
    {
        StatusMessage = TEXT("Load a JSON document and a compatible Skeletal Mesh first.");
        return FReply::Handled();
    }

    bPlaying = !bPlaying;
    PreviewViewport->SetPlaying(bPlaying);
    return FReply::Handled();
}

FReply SArtiMotionStudio::OnStopClicked()
{
    bPlaying = false;
    if (PreviewViewport.IsValid())
    {
        PreviewViewport->SetPlaying(false);
        PreviewViewport->SetPlaybackTime(0.0f);
    }
    return FReply::Handled();
}

void SArtiMotionStudio::OnTimelineChanged(float NewValue)
{
    if (!PreviewViewport.IsValid() || !bDocumentLoaded)
    {
        return;
    }

    bPlaying = false;
    PreviewViewport->SetPlaying(false);
    PreviewViewport->SetPlaybackTime(NewValue * Document.DurationSeconds);
}

FText SArtiMotionStudio::GetStatusText() const
{
    return FText::FromString(StatusMessage);
}

FText SArtiMotionStudio::GetPlayButtonText() const
{
    return bPlaying ? LOCTEXT("Pause", "Pause") : LOCTEXT("Play", "Play");
}

#undef LOCTEXT_NAMESPACE
