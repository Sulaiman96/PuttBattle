// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PuttBattle : ModuleRules
{
	public PuttBattle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"UMG",
			"Niagara",
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystemSteam",
			"Slate",
			"SlateCore"
		});
	}
}
