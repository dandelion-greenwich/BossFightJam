using UnrealBuildTool;
using System.Collections.Generic;

public class BossFightJamEditorTarget : TargetRules
{
	public BossFightJamEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("BossFightJam");
	}
}
