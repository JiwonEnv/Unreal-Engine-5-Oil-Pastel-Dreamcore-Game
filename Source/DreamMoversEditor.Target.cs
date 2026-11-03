using UnrealBuildTool;

public class DreamMoversEditorTarget : TargetRules
{
	public DreamMoversEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DreamMovers");
	}
}
