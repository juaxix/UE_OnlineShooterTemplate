// Juan Belon - 2018

#include "../Public/SPowerUp.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"


ASPowerUp::ASPowerUp()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
}

void ASPowerUp::PowerUpTick()
{
	TicksProcessed++;
	OnPowerUpTick(CharacterPoweredUp);
	if (TicksProcessed >= TotalNrOfTicks)
	{
		bPowerUpIsActive = false;
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);

		OnRep_PowerUpActive(); //<-- BP function
		OnExpirePowerUp(CharacterPoweredUp);
	}
}

void ASPowerUp::ActivatePowerUp(ACharacter* Character)
{
	CharacterPoweredUp = Character;
	bPowerUpIsActive = true;
	OnActivatePowerUp(CharacterPoweredUp);
	OnRep_PowerUpActive(); //<-- BP function

	if (PowerupInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerUp::PowerUpTick, PowerupInterval, true);
	}
	else
	{
		PowerUpTick();
	}
}

void ASPowerUp::OnRep_PowerUpActive()
{
	//BP function
	OnPowerUpStateChange(bPowerUpIsActive, CharacterPoweredUp);
}


void ASPowerUp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASPowerUp, CharacterPoweredUp);
	DOREPLIFETIME(ASPowerUp, bPowerUpIsActive);
}
