// Copyright 2026, Iurii Sernivka.

using UnrealBuildTool;

public class RVR : ModuleRules
{
	public RVR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
		});

		// The public rvr-host-api headers; the implementation is loaded at run time.
		PrivateDependencyModuleNames.Add("RVRHostApi");
	}
}
