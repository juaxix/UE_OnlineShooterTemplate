// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SGameInstance.generated.h"

class ASCharacter;
class UTexture2D;

namespace ENetworkFailure
{
	enum Type;
}

namespace ETravelFailure
{
	enum Type;
}

USTRUCT(BlueprintType)
struct FPlayerInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FText NickName;

	UPROPERTY()
	UTexture2D* AvatarImage;

	UPROPERTY()
	ASCharacter* Character;
};

/**
 *
 */
UCLASS()
class COOPGAME_API USGameInstance : public UGameInstance
{
	GENERATED_BODY()


public:
	UFUNCTION(BlueprintPure, Category = "Game Instance | Session")
	FString NetErrorToString(ENetworkFailure::Type FailureType);
	UFUNCTION(BlueprintPure, Category = "Game Instance | Session")
	FString TravelErrorToString(ETravelFailure::Type FailureType);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Instance | Save Game")
	FName PlayerSettingsSaveName = "PlayerSaveSlot";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Instance | Save Game")
	bool bCreatedSaveFile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Instance | Save Game")
	FPlayerInfo PlayerData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Instance | Session")
	int32 MaxPlayers = 6;

	UPROPERTY()
	FName ServerName = "Server name";
};
