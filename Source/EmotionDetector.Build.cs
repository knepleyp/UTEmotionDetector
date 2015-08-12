// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class EmotionDetector : ModuleRules
	{
        public EmotionDetector(TargetInfo Target)
        {
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "UnrealTournament",
					"InputCore",
					"Slate",
					"SlateCore",
					"ShaderCore",
					"RenderCore",
					"RHI"
				}
				);

            PublicIncludePaths.Add("../../UnrealTournament/Plugins/EmotionDetector/Source/ThirdParty/RSSDK/include");

            var LIBPath = Path.Combine("..", "..", "UnrealTournament", "Plugins", "EmotionDetector", "Source", "ThirdParty", "RSSDK");
            PublicLibraryPaths.Add(LIBPath);

            var RSSDKLibPath = Path.Combine(LIBPath, "libpxc.lib");
            //var RSSDKLibPath = Path.Combine(LIBPath, "libpxcmd.lib");
            
            PublicAdditionalLibraries.Add(RSSDKLibPath);

		}
	}
}