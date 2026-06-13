// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PuttBattle : ModuleRules
{
	public PuttBattle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// The module is organised one subsystem per folder (CONVENTIONS §1) with a
		// flat (no Public/Private) layout, so headers are included by their path
		// relative to the module root (e.g. "Ball/PBBallPawn.h"). Put that root on
		// the include path so those cross-folder includes resolve.
		PublicIncludePaths.Add(ModuleDirectory);

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
			"PhysicsCore",
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
