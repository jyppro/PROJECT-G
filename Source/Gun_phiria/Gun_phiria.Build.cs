// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Gun_phiria : ModuleRules
{
	public Gun_phiria(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "UMG" });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
    }
}
