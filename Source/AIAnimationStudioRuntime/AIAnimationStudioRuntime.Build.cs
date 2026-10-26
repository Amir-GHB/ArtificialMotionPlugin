using UnrealBuildTool;

public class AIAnimationStudioRuntime : ModuleRules
{
    public AIAnimationStudioRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
    }
}