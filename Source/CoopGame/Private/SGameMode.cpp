// Juan Belon - 2018

#include "SGameMode.h"
#include "../Public/SGameStateBase.h"
#include "../Public/SPlayerState.h"
#include "../Public/Components/SHealthComponent.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"


ASGameMode::ASGameMode()
{
	GameStateClass = ASGameStateBase::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();
	PrimaryActorTick.TickInterval = 1.0f; //once a second
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ASGameMode::StartWave()
{
	WaveCount++;
	NumberOfBotsToSpawn = 2 * WaveCount;
	SetWaveState(EWaveStates::IN_PROGRESS);
	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f);
}

void ASGameMode::EndWave()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);

	SetWaveState(EWaveStates::WAITING);
}

void ASGameMode::PrepareForNextWave()
{
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave, TimeBetweenWaves, false);
	SetWaveState(EWaveStates::PREPARING);
	ReStartDeadPlayers();
}

void ASGameMode::CheckWaveState()
{
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart) || NumberOfBotsToSpawn > 0)
	{
		return;
	}

	bool bIsAnyBotAlive = false;
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It && !bIsAnyBotAlive; ++It)
	{
		APawn* pawn = It->Get();
		if (pawn == nullptr || pawn->IsPlayerControlled())
		{
			continue;
		}
		USHealthComponent* HealthComponent = Cast<USHealthComponent>(
			pawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComponent && !HealthComponent->IsDead())
		{
			bIsAnyBotAlive = true;
		}
	}

	if (!bIsAnyBotAlive)
	{
		SetWaveState(EWaveStates::COMPLETED);
		PrepareForNextWave();
	}
}

void ASGameMode::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* playerController = It->Get();
		APawn* playerPawn = nullptr;
		if (playerController && (playerPawn = playerController->GetPawn()) != nullptr)
		{
			USHealthComponent* healthComponent = Cast<USHealthComponent>(
				playerPawn->GetComponentByClass(USHealthComponent::StaticClass()));
			if (healthComponent && !healthComponent->IsDead())
			{
				return;
			}
		}
	}

	GameOver();
}

void ASGameMode::SpawnBotTimerElapsed()
{
	SpawnNewBot();
	NumberOfBotsToSpawn--;
	UE_LOG(LogTemp, Log, TEXT("Bots left to spawn %d"), NumberOfBotsToSpawn);
	if (NumberOfBotsToSpawn <= 0)
	{
		EndWave();
	}
}

void ASGameMode::GameOver()
{
	EndWave();
	//@TODO: finish up the match, present 'game over' to players
	UE_LOG(LogTemp, Log, TEXT("GAME OVER!: All players died"));
	SetWaveState(EWaveStates::END);
}

void ASGameMode::SetWaveState(EWaveStates NewState)
{
	ASGameStateBase* gamestate = GetGameState<ASGameStateBase>();
	if (ensureAlways(gamestate))
	{
		gamestate->ChangeWaveState(NewState);
	}
}


void ASGameMode::ReStartDeadPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* playerController = It->Get();
		APawn* playerPawn = nullptr;
		if (playerController && (playerPawn = playerController->GetPawn()) == nullptr)
		{
			RestartPlayer(playerController);
		}
	}
}

void ASGameMode::StartPlay()
{
	Super::StartPlay();
	PrepareForNextWave();
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckWaveState();
	CheckAnyPlayerAlive();
}
