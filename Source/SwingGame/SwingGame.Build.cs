// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SwingGame : ModuleRules
{
	public SwingGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SwingGame",
			"SwingGame/Variant_Platforming",
			"SwingGame/Variant_Platforming/Animation",
			"SwingGame/Variant_Combat",
			"SwingGame/Variant_Combat/AI",
			"SwingGame/Variant_Combat/Animation",
			"SwingGame/Variant_Combat/Gameplay",
			"SwingGame/Variant_Combat/Interfaces",
			"SwingGame/Variant_Combat/UI",
			"SwingGame/Variant_SideScrolling",
			"SwingGame/Variant_SideScrolling/AI",
			"SwingGame/Variant_SideScrolling/Gameplay",
			"SwingGame/Variant_SideScrolling/Interfaces",
			"SwingGame/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
