#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "SArtiMotionStudio.h"

#define LOCTEXT_NAMESPACE "FAIAnimationStudioEditorModule"

class FAIAnimationStudioEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
            TabName,
            FOnSpawnTab::CreateRaw(this, &FAIAnimationStudioEditorModule::SpawnStudioTab))
            .SetDisplayName(LOCTEXT("TabTitle", "ArtiMotion"))
            .SetTooltipText(LOCTEXT("TabTooltip", "Open the ArtiMotion animation authoring workspace."));
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
                SNew(SArtiMotionStudio)
            ];
    }
};

const FName FAIAnimationStudioEditorModule::TabName(TEXT("ArtiMotion"));

IMPLEMENT_MODULE(FAIAnimationStudioEditorModule, AIAnimationStudioEditor)

#undef LOCTEXT_NAMESPACE
