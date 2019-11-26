// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class CodeEditor : ModuleRules
	{
		public CodeEditor(ReadOnlyTargetRules Target) : base(Target)
		{
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PrivateIncludePaths.AddRange(
				new string[] {
					"CodeEditor/Private/Assets",
				}
				);

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "EditorScriptingUtilities",
                }
                );


            PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"SlateCore",
					"Slate",
					"AssetTools",
                    "AssetRegistry",
                    "Engine",
                    "UnrealEd",
					"EditorStyle",
					"PropertyEditor",
					"Kismet",  // for FWorkflowCentricApplication
                    "Projects",
                    "InputCore",
					"DirectoryWatcher",
					"LevelEditor",
                    "ScriptHelper",
				}
				);
		}
	}
}
