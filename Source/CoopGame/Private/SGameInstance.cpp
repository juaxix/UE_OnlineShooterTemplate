// Juan Belon - 2018

#include "SGameInstance.h"
#include "Net/UnrealNetwork.h"


FString USGameInstance::NetErrorToString(ENetworkFailure::Type FailureType)
{
	return ENetworkFailure::ToString(FailureType);
}

FString USGameInstance::TravelErrorToString(ETravelFailure::Type FailureType)
{
	return ETravelFailure::ToString(FailureType);
}
