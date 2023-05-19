// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ContextualDialogueEditor : ModuleRules
{
	public ContextualDialogueEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Recursively add all 'Public' include directories as public include paths
		string[] Dirs = Directory.GetDirectories(ModuleDirectory, "Public", SearchOption.AllDirectories);
		PublicIncludePaths.AddRange(Dirs);
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Blutility",
			"ContextualDialogue"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"AssetTools",
			"EditorWidgets",
			"UnrealEd",
			"ComponentVisualizers",
			"UMGEditor",
			"UMG",
			"Json"
		});
	}
}
