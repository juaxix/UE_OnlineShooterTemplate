// Juan Belon - 2018

#include "SGameStateBase.h"
#include "Net/UnrealNetwork.h"

void ASGameStateBase::OnRep_WaveState(EWaveStates PreviousState)
{
	OnWaveStateChanged(WaveState, PreviousState);
}

void ASGameStateBase::ChangeWaveState(EWaveStates NewState)
{
	if (HasAuthority())
	{
		EWaveStates oldstate = WaveState;
		WaveState = NewState;
		OnRep_WaveState(oldstate); //trigger for server 
	}
}


void ASGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASGameStateBase, WaveState);
}
