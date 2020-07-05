// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerUp.generated.h"

class ACharacter;

UCLASS()
class COOPGAME_API ASPowerUp : public AActor
{
	GENERATED_BODY()

public:

	ASPowerUp();

protected:

	UFUNCTION(Category="PowerUps")
	void PowerUpTick();

public:
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	void ActivatePowerUp(ACharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "PowerUps")
	void OnActivatePowerUp(ACharacter* ForCharacter);

	UFUNCTION(BlueprintImplementableEvent, Category = "PowerUps")
	void OnExpirePowerUp(ACharacter* ForCharacter);

	UFUNCTION(BlueprintImplementableEvent, Category = "PowerUps")
	void OnPowerUpTick(ACharacter* ForCharacter);

	UFUNCTION(Category="PowerUps")
	void OnRep_PowerUpActive();

	UFUNCTION(BlueprintImplementableEvent, Category="PowerUps")
	void OnPowerUpStateChange(bool IsActive, ACharacter* ForCharacter);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PowerUps", meta= (ToolTip="Time between powerup ticks"))
	float PowerupInterval = 0.0f; //by default it will not tick

	UPROPERTY(EditDefaultsOnly, Category = "PowerUps", meta=(ToolTip ="Total times we apply the powerup effect"))
	int32 TotalNrOfTicks = 1;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_PowerUpActive, Category="PowerUps")
	bool bPowerUpIsActive = false;

	UPROPERTY(VisibleInstanceOnly,Replicated, Category="PowerUps")
	ACharacter* CharacterPoweredUp = nullptr;

	int32 TicksProcessed = 0;
	FTimerHandle TimerHandle_PowerupTick;
};
