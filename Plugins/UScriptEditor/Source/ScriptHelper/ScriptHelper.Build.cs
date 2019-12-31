// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ScriptHelper : ModuleRules
	{
        public ScriptHelper(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(new string[]
			{
                System.IO.Path.Combine(ModuleDirectory, "Public"),
                System.IO.Path.Combine(ModuleDirectory, "Private"),
            });


            PrivateIncludePaths.AddRange(new string[] { "ScriptHelper/Private" });

            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "SlateCore",
                    "UMG",
                }
			);
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "AssetRegistry",
                    "UnLua",
                }
            );

        }
	}
}