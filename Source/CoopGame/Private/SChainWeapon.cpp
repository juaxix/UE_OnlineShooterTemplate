// Juan Belon - 2018

#include "../Public/SChainWeapon.h"
#include "../Public/SWeapon.h"
#include "../Public/Components/SHealthComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "Sound/SoundCue.h"

ASChainWeapon::ASChainWeapon()
{
	ChainEffectComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ChainEffectComponent"));
	ChainEffectComponent->SetupAttachment(SkeletalMeshComponent, MuzzleSocketName);
	//ChainEffectComponent->SetIsReplicated(true);
}

void ASChainWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (ChainEffect)
	{
		ChainEffectComponent->SetTemplate(ChainEffect);
		OnStoppedFire();
	}
}

void ASChainWeapon::StopFire()
{
	Super::StopFire();
	if (HasAuthority())
	{
		HitScanTrace.ImpactPoint = FVector::ZeroVector;
	}
}

void ASChainWeapon::PlayFireEffects(FVector EndPoint, FRotator EndRotation, EPhysicalSurface SurfaceType)
{
	Super::PlayFireEffects(EndPoint, EndRotation, SurfaceType);

	if (!EndPoint.Equals(FVector::ZeroVector))
	{
		ChainEffectComponent->SetBeamTargetPoint(0, EndPoint, 0);
	}
}

void ASChainWeapon::OnStartedFire()
{
	Super::OnStartedFire();
	ChainEffectComponent->ActivateSystem(false);
	ChainEffectComponent->SetVisibility(true);
	OnFireStartedEvent.Broadcast();

	if (!HasAuthority())
	{
		//Make sure the client starts fire from server
		Server_StartedFire();
	}
}

void ASChainWeapon::Server_StartedFire_Implementation()
{
	OnStartedFire();
}

bool ASChainWeapon::Server_StartedFire_Validate()
{
	return true;
}

void ASChainWeapon::Server_StoppedFire_Implementation()
{
	OnStoppedFire();
}

bool ASChainWeapon::Server_StoppedFire_Validate()
{
	return true;
}

void ASChainWeapon::OnStoppedFire()
{
	Super::OnStoppedFire();
	ChainEffectComponent->DeactivateSystem();
	ChainEffectComponent->SetVisibility(false);
	OnFireEndedEvent.Broadcast();
	if (!HasAuthority())
	{
		//make sure the client stops fire from server
		Server_StoppedFire();
	}
}
