// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ScriptParamCollector : ModuleRules
	{
        public ScriptParamCollector(ReadOnlyTargetRules Target) : base(Target)
        {
            bEnforceIWYU = false;

            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
                new string[] {
                    "Programs/UnrealHeaderTool/Public",
                }
                );


            PrivateIncludePaths.AddRange(
                new string[] {
                    "ScriptParamCollertor/Private",
                }
                );


            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                }
                );

            PublicDefinitions.Add("HACK_HEADER_GENERATOR=1");

        }
	}
}