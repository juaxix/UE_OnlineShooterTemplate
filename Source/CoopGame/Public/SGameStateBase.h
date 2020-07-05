// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SGameStateBase.generated.h"

UENUM(BlueprintType)
enum class EWaveStates : uint8
{
	IDLE UMETA(DisplayName = "Idle"),
	PREPARING UMETA(DisplayName = "Preparing Next Wave"),
	IN_PROGRESS UMETA(DisplayName = "Wave in Progress"),
	WAITING UMETA(DisplayName = "Waiting to complete"),
	COMPLETED UMETA(DisplayName = "Wave complete"),
	END UMETA(DisplayName = "Game Over")
};

/**
 *
 */
UCLASS()
class COOPGAME_API ASGameStateBase : public AGameStateBase
{
	GENERATED_BODY()


protected:
	UFUNCTION(Category="Game State")
	void OnRep_WaveState(EWaveStates PreviousState);


	UFUNCTION(BlueprintImplementableEvent, Category = "Game State")
	void OnWaveStateChanged(EWaveStates NewState, EWaveStates OldState);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WaveState, Category = "Game State")
	EWaveStates WaveState = EWaveStates::IDLE;

public:
	void ChangeWaveState(EWaveStates NewState);
};
