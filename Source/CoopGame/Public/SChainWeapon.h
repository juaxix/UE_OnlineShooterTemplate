// Juan Belon - 2018

#pragma once
#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SChainWeapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFireStartedEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFireEndedEvent);

class UParticleSystem;
class UParticleSystemComponent;

UCLASS()
class COOPGAME_API ASChainWeapon : public ASWeapon
{
	GENERATED_BODY()

public:

	ASChainWeapon();
	virtual void BeginPlay() override;
	virtual void StopFire() override;
	virtual void PlayFireEffects(FVector EndPoint = FVector::ZeroVector, FRotator EndRotation = FRotator::ZeroRotator,
								 EPhysicalSurface SurfaceType = EPhysicalSurface::SurfaceType_Default) override;

	virtual void OnStartedFire() override;
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartedFire();

	virtual void OnStoppedFire() override;
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StoppedFire();

protected:
	UPROPERTY(EditAnywhere, Category="Chain Weapon")
	UParticleSystem* ChainEffect = nullptr;

	UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category="Chain Weapon")
	UParticleSystemComponent* ChainEffectComponent;

	UPROPERTY(BlueprintAssignable, Category = "Chain Weapon")
	FOnFireStartedEvent OnFireStartedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Chain Weapon")
	FOnFireEndedEvent OnFireEndedEvent;
};
