#include "AnimationDocumentJson.h"
#include "AnimationDocumentValidator.h"
#include "Dom/JsonObject.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace ArtiMotionJson
{
    bool ReadString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FString& Out, TArray<FString>& Errors, bool bRequired = true)
    {
        if (Object.IsValid() && Object->TryGetStringField(Field, Out)) return true;
        if (bRequired) Errors.Add(FString::Printf(TEXT("Missing or invalid string field '%s'."), Field));
        Out.Reset();
        return !bRequired;
    }

    bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, double& Out, TArray<FString>& Errors)
    {
        if (Object.IsValid() && Object->TryGetNumberField(Field, Out)) return true;
        Errors.Add(FString::Printf(TEXT("Missing or invalid numeric field '%s'."), Field));
        return false;
    }

    bool ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& Out, TArray<FString>& Errors)
    {
        const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, A) || !A || A->Num() != 3)
        {
            Errors.Add(FString::Printf(TEXT("Field '%s' must be an array of three numbers."), Field));
            return false;
        }
        double V[3] = {0, 0, 0};
        for (int32 I = 0; I < 3; ++I)
        {
            if (!(*A)[I].IsValid() || !(*A)[I]->TryGetNumber(V[I]))
            {
                Errors.Add(FString::Printf(TEXT("Field '%s' contains a non-numeric component."), Field));
                return false;
            }
        }
        Out = FVector(V[0], V[1], V[2]);
        return true;
    }

    bool ReadQuat(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FQuat& Out, TArray<FString>& Errors)
    {
        const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, A) || !A || A->Num() != 4)
        {
            Errors.Add(FString::Printf(TEXT("Field '%s' must be [x, y, z, w]."), Field));
            return false;
        }
        double V[4] = {0, 0, 0, 1};
        for (int32 I = 0; I < 4; ++I)
        {
            if (!(*A)[I].IsValid() || !(*A)[I]->TryGetNumber(V[I]))
            {
                Errors.Add(FString::Printf(TEXT("Field '%s' contains a non-numeric component."), Field));
                return false;
            }
        }
        Out = FQuat(V[0], V[1], V[2], V[3]);
        return true;
    }

    TArray<TSharedPtr<FJsonValue>> VectorArray(const FVector& V)
    {
        return { MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y), MakeShared<FJsonValueNumber>(V.Z) };
    }

    TArray<TSharedPtr<FJsonValue>> QuatArray(const FQuat& Q)
    {
        return { MakeShared<FJsonValueNumber>(Q.X), MakeShared<FJsonValueNumber>(Q.Y), MakeShared<FJsonValueNumber>(Q.Z), MakeShared<FJsonValueNumber>(Q.W) };
    }

    bool ParseRootMotion(const FString& S, EAIAnimationRootMotionMode& Out)
    {
        if (S == TEXT("in_place")) { Out = EAIAnimationRootMotionMode::InPlace; return true; }
        if (S == TEXT("extract_from_root")) { Out = EAIAnimationRootMotionMode::ExtractFromRoot; return true; }
        if (S == TEXT("preserve_root_track")) { Out = EAIAnimationRootMotionMode::PreserveRootTrack; return true; }
        return false;
    }

    const TCHAR* RootMotionString(EAIAnimationRootMotionMode Mode)
    {
        switch (Mode)
        {
        case EAIAnimationRootMotionMode::ExtractFromRoot: return TEXT("extract_from_root");
        case EAIAnimationRootMotionMode::PreserveRootTrack: return TEXT("preserve_root_track");
        default: return TEXT("in_place");
        }
    }
}

bool FArtiMotionDocumentJson::ParseAndValidate(
    const FString& JsonString,
    FAIAnimationDocument& OutDocument,
    TArray<FString>& OutErrors,
    const TArray<FName>& ValidBoneNames)
{
    OutErrors.Reset();
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutErrors.Add(FString::Printf(TEXT("Invalid JSON: %s"), *Reader->GetErrorMessage()));
        return false;
    }

    FAIAnimationDocument Parsed;
    FString S;
    double N = 0.0;
    ReadString(Root, TEXT("schemaVersion"), Parsed.SchemaVersion, OutErrors);
    ReadString(Root, TEXT("name"), Parsed.Name, OutErrors);
    if (ReadNumber(Root, TEXT("revision"), N, OutErrors)) Parsed.Revision = static_cast<int32>(N);
    if (ReadNumber(Root, TEXT("durationSeconds"), N, OutErrors)) Parsed.DurationSeconds = static_cast<float>(N);
    if (ReadNumber(Root, TEXT("sampleRate"), N, OutErrors)) Parsed.SampleRate = static_cast<int32>(N);

    if (ReadString(Root, TEXT("documentId"), S, OutErrors, false))
    {
        FGuid Guid;
        Parsed.DocumentId = FGuid::Parse(S, Guid) ? Guid : FGuid::NewGuid();
    }
    if (ReadString(Root, TEXT("skeletonAssetPath"), S, OutErrors, false) && !S.IsEmpty())
        Parsed.SkeletonAssetPath = FSoftObjectPath(S);
    ReadString(Root, TEXT("skeletonSignature"), Parsed.SkeletonSignature, OutErrors, false);

    if (ReadString(Root, TEXT("rootMotionMode"), S, OutErrors))
    {
        if (!ParseRootMotion(S, Parsed.RootMotionMode))
            OutErrors.Add(FString::Printf(TEXT("Unknown rootMotionMode '%s'."), *S));
    }
    if (ReadString(Root, TEXT("rootBoneName"), S, OutErrors, false) && !S.IsEmpty())
        Parsed.RootBoneName = FName(*S);

    const TArray<TSharedPtr<FJsonValue>>* Tracks = nullptr;
    if (!Root->TryGetArrayField(TEXT("tracks"), Tracks) || !Tracks)
    {
        OutErrors.Add(TEXT("Missing or invalid array field 'tracks'."));
    }
    else
    {
        for (int32 TI = 0; TI < Tracks->Num(); ++TI)
        {
            const TSharedPtr<FJsonObject> TrackObject = (*Tracks)[TI].IsValid() ? (*Tracks)[TI]->AsObject() : nullptr;
            if (!TrackObject.IsValid())
            {
                OutErrors.Add(FString::Printf(TEXT("Track %d must be an object."), TI));
                continue;
            }
            FAIAnimationBoneTrack Track;
            FString Bone;
            if (!ReadString(TrackObject, TEXT("boneName"), Bone, OutErrors)) continue;
            Track.BoneName = FName(*Bone);

            const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
            if (!TrackObject->TryGetArrayField(TEXT("keys"), Keys) || !Keys)
            {
                OutErrors.Add(FString::Printf(TEXT("Track '%s' is missing its keys array."), *Bone));
                continue;
            }
            for (int32 KI = 0; KI < Keys->Num(); ++KI)
            {
                const TSharedPtr<FJsonObject> KeyObject = (*Keys)[KI].IsValid() ? (*Keys)[KI]->AsObject() : nullptr;
                if (!KeyObject.IsValid())
                {
                    OutErrors.Add(FString::Printf(TEXT("Track '%s' key %d must be an object."), *Bone, KI));
                    continue;
                }
                FAIAnimationKeyframe Key;
                if (ReadNumber(KeyObject, TEXT("timeSeconds"), N, OutErrors)) Key.TimeSeconds = static_cast<float>(N);
                ReadVector(KeyObject, TEXT("translation"), Key.Translation, OutErrors);
                ReadQuat(KeyObject, TEXT("rotation"), Key.Rotation, OutErrors);
                ReadVector(KeyObject, TEXT("scale"), Key.Scale, OutErrors);
                Track.Keys.Add(Key);
            }
            Parsed.Tracks.Add(MoveTemp(Track));
        }
    }

    if (!OutErrors.IsEmpty() || !FAIAnimationDocumentValidator::Validate(Parsed, OutErrors, ValidBoneNames)) return false;
    OutDocument = MoveTemp(Parsed);
    return true;
}

bool FArtiMotionDocumentJson::ToJson(
    const FAIAnimationDocument& Document,
    FString& OutJson,
    TArray<FString>& OutErrors,
    bool bPrettyPrint)
{
    OutErrors.Reset();
    if (!FAIAnimationDocumentValidator::Validate(Document, OutErrors)) return false;

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("schemaVersion"), Document.SchemaVersion);
    Root->SetStringField(TEXT("documentId"), (Document.DocumentId.IsValid() ? Document.DocumentId : FGuid::NewGuid()).ToString(EGuidFormats::DigitsWithHyphens));
    Root->SetNumberField(TEXT("revision"), Document.Revision);
    Root->SetStringField(TEXT("name"), Document.Name);
    Root->SetStringField(TEXT("skeletonAssetPath"), Document.SkeletonAssetPath.ToString());
    Root->SetStringField(TEXT("skeletonSignature"), Document.SkeletonSignature);
    Root->SetNumberField(TEXT("durationSeconds"), Document.DurationSeconds);
    Root->SetNumberField(TEXT("sampleRate"), Document.SampleRate);
    Root->SetStringField(TEXT("rootMotionMode"), RootMotionString(Document.RootMotionMode));
    Root->SetStringField(TEXT("rootBoneName"), Document.RootBoneName.ToString());

    TArray<TSharedPtr<FJsonValue>> TrackValues;
    for (const FAIAnimationBoneTrack& Track : Document.Tracks)
    {
        TSharedRef<FJsonObject> TrackObject = MakeShared<FJsonObject>();
        TrackObject->SetStringField(TEXT("boneName"), Track.BoneName.ToString());
        TArray<TSharedPtr<FJsonValue>> KeyValues;
        for (const FAIAnimationKeyframe& Key : Track.Keys)
        {
            TSharedRef<FJsonObject> KeyObject = MakeShared<FJsonObject>();
            KeyObject->SetNumberField(TEXT("timeSeconds"), Key.TimeSeconds);
            KeyObject->SetArrayField(TEXT("translation"), VectorArray(Key.Translation));
            KeyObject->SetArrayField(TEXT("rotation"), QuatArray(Key.Rotation));
            KeyObject->SetArrayField(TEXT("scale"), VectorArray(Key.Scale));
            KeyValues.Add(MakeShared<FJsonValueObject>(KeyObject));
        }
        TrackObject->SetArrayField(TEXT("keys"), KeyValues);
        TrackValues.Add(MakeShared<FJsonValueObject>(TrackObject));
    }
    Root->SetArrayField(TEXT("tracks"), TrackValues);

    if (bPrettyPrint)
    {
        const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
            TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutJson);
        if (!FJsonSerializer::Serialize(Root, Writer))
        {
            OutErrors.Add(TEXT("Failed to serialize animation document."));
            return false;
        }
        return true;
    }
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        OutErrors.Add(TEXT("Failed to serialize animation document."));
        return false;
    }
    return true;
}
