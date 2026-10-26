using UnrealBuildTool;

public class AIAnimationStudioCore : ModuleRules
{
    public AIAnimationStudioCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Json",
            "AIAnimationStudioRuntime"
        });
    }
}
