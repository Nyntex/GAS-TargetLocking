// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestingProject : ModuleRules
{
	public TestingProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "GameplayAbilities", "TargetLock" });
	}
}
