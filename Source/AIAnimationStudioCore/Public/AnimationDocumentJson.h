#pragma once

#include "CoreMinimal.h"
#include "AnimationDocumentTypes.h"

/** Strict reader/writer for the canonical ArtiMotion animation JSON schema. */
class AIANIMATIONSTUDIOCORE_API FArtiMotionDocumentJson
{
public:
    static bool ParseAndValidate(
        const FString& JsonString,
        FAIAnimationDocument& OutDocument,
        TArray<FString>& OutErrors,
        const TArray<FName>& ValidBoneNames = TArray<FName>());

    static bool ToJson(
        const FAIAnimationDocument& Document,
        FString& OutJson,
        TArray<FString>& OutErrors,
        bool bPrettyPrint = true);
};
