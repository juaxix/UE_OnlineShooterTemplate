// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"

enum class EWaveStates : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilled, AActor*, VictimActor, AActor*, KillerActor, AController*,
											   KillerController);

/**
 * Server class to handle the logic of the game
 */
UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ASGameMode();
protected:

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode", meta =(ToolTip="Hook for BP to spawn a bot"))
	void SpawnNewBot();

	///start spawning bots
	void StartWave();

	///stop spawning bots
	void EndWave();

	///set timer for next wave
	void PrepareForNextWave();

	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void SpawnBotTimerElapsed();

	void GameOver();

	void SetWaveState(EWaveStates NewState);

	void ReStartDeadPlayers();

public:

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
protected:
	UPROPERTY(VisibleInstanceOnly, Category="Game Mode", meta=(Tooltip="Bots to spawn in the current wave"))
	int32 NumberOfBotsToSpawn = 2;

	int32 WaveCount = 0;

	FTimerHandle TimerHandle_NextWaveStart;
	FTimerHandle TimerHandle_BotSpawner;

	UPROPERTY(EditAnywhere, Category="Game Mode")
	float TimeBetweenWaves = 2.0f;
public:
	UPROPERTY(BlueprintAssignable, Category = "Game Mode")
	FOnActorKilled OnActorKilled;
};
