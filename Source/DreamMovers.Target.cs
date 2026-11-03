using UnrealBuildTool;

public class DreamMoversTarget : TargetRules
{
	public DreamMoversTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DreamMovers");
	}
}
