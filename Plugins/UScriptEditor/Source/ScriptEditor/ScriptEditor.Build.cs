﻿// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ScriptEditor : ModuleRules
	{
		public ScriptEditor(ReadOnlyTargetRules Target) : base(Target)
		{
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
                new string[]{
                    System.IO.Path.Combine(ModuleDirectory, "Public"),
                }
                );

            PrivateIncludePaths.AddRange(
				new string[] {
					"ScriptEditor/Private/Assets",
                    "ScriptEditor/Private/CodeEditor",
                    "ScriptEditor/Private/ProjectEditor",
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