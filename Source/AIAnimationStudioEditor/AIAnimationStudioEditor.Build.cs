using UnrealBuildTool;

public class AIAnimationStudioEditor : ModuleRules
{
    public AIAnimationStudioEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "UnrealEd",
            "AIAnimationStudioRuntime",
            "AIAnimationStudioCore"
        });
    }
}