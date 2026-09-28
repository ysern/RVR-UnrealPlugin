// Copyright 2026, Iurii Sernivka.

#include "RVRSettings.h"

#include "RVR.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RVRSettings)

TArray<FString> URVRSettings::GetLobbyNames() const
{
	TArray<FString> Names;
	if (FRVRModule::IsAvailable())
	{
		for (const FRVRLobby& L : FRVRModule::Get().GetStatus().Lobbies)
		{
			Names.Add(L.Name);
		}
	}
	return Names;
}
