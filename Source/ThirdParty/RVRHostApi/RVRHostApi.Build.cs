// Copyright 2026, Iurii Sernivka.

using System.IO;
using UnrealBuildTool;

// rvr-host-api: the public C interface and its loader, headers only. No binary of
// RVR's is linked or shipped here: the implementation is installed with RVR and
// loaded at run time, so the plugin builds from these headers alone.
public class RVRHostApi : ModuleRules
{
	public RVRHostApi(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
	}
}
