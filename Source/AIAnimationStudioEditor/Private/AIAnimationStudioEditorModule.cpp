#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FAIAnimationStudioEditorModule"

class FAIAnimationStudioEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
            TabName,
            FOnSpawnTab::CreateRaw(this, &FAIAnimationStudioEditorModule::SpawnStudioTab))
            .SetDisplayName(LOCTEXT("TabTitle", "AI Animation Studio"))
            .SetTooltipText(LOCTEXT("TabTooltip", "Open the AI-assisted animation authoring workspace."));
    }

    virtual void ShutdownModule() override
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
    }

private:
    static const FName TabName;

    TSharedRef<SDockTab> SpawnStudioTab(const FSpawnTabArgs& Args)
    {
        return SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(STextBlock)
                .Text(LOCTEXT(
                    "ScaffoldMessage",
                    "AI Animation Studio\n\nPlugin scaffold loaded. Next: animation document preview."))
            ];
    }
};

const FName FAIAnimationStudioEditorModule::TabName(TEXT("AIAnimationStudio"));

IMPLEMENT_MODULE(FAIAnimationStudioEditorModule, AIAnimationStudioEditor)

#undef LOCTEXT_NAMESPACE