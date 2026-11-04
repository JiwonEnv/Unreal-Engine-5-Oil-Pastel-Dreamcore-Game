using UnrealBuildTool;

public class DreamMovers : ModuleRules
{
	public DreamMovers(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "SlateCore",
			"AIModule", "NavigationSystem", "GameplayTasks", "ProcLevelGen"
		});
	}
}
