// Copyright 2026, Iurii Sernivka.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "RVRSettings.generated.h"

/**
 * Project Settings > Plugins > RVR. Saved with the project. A setting left empty
 * falls back to RVR's own configuration on this machine.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "RVR"))
class RVR_API URVRSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** The Signalling Server, e.g. https://signal.example.com. Empty: RVR's own configuration. */
	UPROPERTY(Config, EditAnywhere, Category = "Signalling", meta = (DisplayName = "Signalling Server"))
	FString SignallingServer;

	/** A Connection Profile file, as a Tenant Administrator downloads it. Used instead of the server above. */
	UPROPERTY(Config, EditAnywhere, Category = "Signalling", meta = (FilePathFilter = "json"))
	FFilePath ConnectionProfile;

	/** The Lobby to be present in, by name: identifiers differ between deployments. */
	UPROPERTY(Config, EditAnywhere, Category = "Signalling", meta = (GetOptions = "GetLobbyNames"))
	FString Lobby;

	/** Go online, and open the Host Session, when the Editor opens this project. */
	UPROPERTY(Config, EditAnywhere, Category = "Signalling")
	bool bGoOnlineAtStartup = false;

	/** The names of the Lobbies RVR reports this machine may use. */
	UFUNCTION()
	TArray<FString> GetLobbyNames() const;

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("RVR"); }
};
