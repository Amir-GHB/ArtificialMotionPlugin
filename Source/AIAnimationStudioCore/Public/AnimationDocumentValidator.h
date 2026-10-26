#pragma once

#include "CoreMinimal.h"
#include "AnimationDocumentTypes.h"

/** Stateless validation for editable animation documents and AI-generated data. */
class AIANIMATIONSTUDIOCORE_API FAIAnimationDocumentValidator
{
public:
    /**
     * Validates structure and numeric data.
     * If ValidBoneNames is non-empty, each track must reference a listed bone.
     */
    static bool Validate(
        const FAIAnimationDocument& Document,
        TArray<FString>& OutErrors,
        const TArray<FName>& ValidBoneNames = TArray<FName>());
};