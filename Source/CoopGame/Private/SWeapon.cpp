// Juan Belon - 2018
#include "../Public/SWeapon.h"
#include "CoopGame.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "Sound/SoundCue.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
TAutoConsoleVariable<int32> CVarDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"),
	0,
	TEXT("Enable or Disable debug tracers for weapons and projectiles\n")
	TEXT("0 = off\n")
	TEXT("1 = Draw Debug Tracers"),
	ECVF_Cheat);

#endif

ASWeapon::ASWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	RootComponent = Cast<USceneComponent>(SkeletalMeshComponent);
	SetReplicates(true);
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}


void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
	TimeBetweenShots = 60.0f / FireRate;
	LastFireTime = -TimeBetweenShots;
}

void ASWeapon::Fire()
{
	if (!CheckAmmo())
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerFire();
	}

	//Trace the world, from pawn eyes to cross hair location
	AActor* owner = GetOwner();
	if (owner)
	{
		FHitResult Hit;
		FVector EyeLocation;
		FRotator EyeRotation;
		FRotator EndRotation = FRotator::ZeroRotator;
		owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		FVector ShotDirection = EyeRotation.Vector();

		//--- direction in the shape of a cone
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);
		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		//Particle target parameter
		FVector TracerEndPoint = TraceEnd;
		FCollisionQueryParams ColQueryParams;
		EPhysicalSurface SurfaceType = EPhysicalSurface::SurfaceType_Default;
		ColQueryParams.AddIgnoredActor(owner);
		ColQueryParams.AddIgnoredActor(this);
		ColQueryParams.bReturnPhysicalMaterial = true;

		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, ColQueryParams))
		{
			// blocking hit! -> process damage
			AActor* HitActor = Hit.GetActor();
			TracerEndPoint = Hit.ImpactPoint;
			EndRotation = Hit.ImpactNormal.Rotation();
			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			if (HasAuthority())
			{
				float ActualDamage = SurfaceType == SURFACE_FLESH_VULNERABLE ? BaseDamage * 4.0f : BaseDamage;
				UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit,
											   owner->GetInstigatorController(), owner, DamageType);
			}
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDebugWeaponDrawing.GetValueOnAnyThread(false) > 0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TracerEndPoint, FColor::Green, false, 1.0f, 0, 1.0f);
		}

#endif

		if (HasAuthority())
		{
			PlayFireEffects(TracerEndPoint, EndRotation, SurfaceType);
			HitScanTrace.TraceFrom = EyeLocation;
			HitScanTrace.ImpactPoint = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;
		CurrentChamberAmmo--;
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

bool ASWeapon::CheckAmmo()
{
	if (bIsReloading)
	{
		return false;
	}

	if (!HasAmmo())
	{
		if (CanReloadAmmo())
		{
			Reload();
		}
		else
		{
			//No ammo
			StopFire();
		}

		return false;
	}

	return true;
}

void ASWeapon::StartFire()
{
	if (bIsFiring)
	{
		return;
	}

	const float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->GetTimeSeconds(), 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true,
									FirstDelay);

	bIsFiring = true;
	OnRep_IsFiring();
}

void ASWeapon::StopFire()
{
	if (!bIsFiring)
	{
		return;
	}

	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(TimerHandle_TimeBetweenShots);
	//TimerManager.ClearTimer(TimerHandle_Reloading);

	bIsFiring = false;
	OnRep_IsFiring();
}

void ASWeapon::PlayFireEffects(FVector EndPoint, FRotator EndRotation, EPhysicalSurface SurfaceType)
{
	const FVector& MuzzleLocation = SkeletalMeshComponent->GetSocketLocation(MuzzleSocketName);
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleEffect, MuzzleLocation,
												 SkeletalMeshComponent->GetSocketRotation(MuzzleSocketName));
	}

	if (EndPoint != FVector::ZeroVector)
	{
		//Particles
		UParticleSystem* ParticleSystem;

		//Get Physics properties:
		//  TWeakPtr is not a direct pointer, it's a weak reference, we tell the engine it can delete the pointer if we are
		//  the last one to use it
		switch (SurfaceType)
		{
		case SURFACE_FLESH_DEFAULT:
		case SURFACE_FLESH_VULNERABLE:
			ParticleSystem = FleshImpactEffect;
			if (HitFireEnemySound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitFireEnemySound, EndPoint);
			}
			break;
		default:
			if (HitFireSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitFireSound, EndPoint);
			}
			ParticleSystem = ImpactEffect;
			break;
		}

		if (ParticleSystem)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleSystem, EndPoint, EndRotation);
		}

		//Tracer
		UParticleSystemComponent* TracerComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), TracerEffect, SkeletalMeshComponent->GetSocketLocation(MuzzleSocketName));

		if (TracerComponent != nullptr)
		{
			TracerComponent->SetVectorParameter(TracerParamName /*name in the particle system*/, EndPoint);
		}
	}

	if(bPlayStartSoundPerProjectile && StartFireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StartFireSound, GetActorLocation());
	}

	//Camera shake
	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCameraShake);
		}
	}
}

void ASWeapon::Reload()
{
	if (bIsReloading || !CanReloadAmmo())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(TimerHandle_Reloading, [this]()
	{
		//Actual reload: approach from games like overwatch
		CurrentChamberAmmo = MaxChamberAmmo;

		/**
		 * // approach from games like apex:legends, you can only fire when you have enough --limited-- ammo
		 * // this is the other method if you want to use a limited amount of ammo instead of infinite reloads
		 *  const int32 oldammo = CurrentChamberAmmo;
			int32 newammo = MaxChamberAmmo - oldammo;
			CurrentChamberAmmo = TotalAmmo >= newammo ? newammo : TotalAmmo;
			TotalAmmo -= newammo;
		 */

		bIsReloading = false;
	}, TimeToReload, false, TimeToReload);

	bIsReloading = true;

	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
	}
}

void ASWeapon::OnRep_HitScanTrace(FHitScanTrace OldHitScanTrace)
{
	//Play cosmetic FX
	FVector ShotDirection = HitScanTrace.ImpactPoint - HitScanTrace.TraceFrom;
	ShotDirection.Normalize();

	PlayFireEffects(HitScanTrace.ImpactPoint, ShotDirection.Rotation(), HitScanTrace.SurfaceType);
}

void ASWeapon::OnRep_IsFiring()
{
	if (bIsFiring)
	{
		OnStartedFire();
	}
	else
	{
		OnStoppedFire();
	}
}

void ASWeapon::OnStartedFire()
{
	if(!bPlayStartSoundPerProjectile && StartFireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, StartFireSound, GetActorLocation());
	}
}

void ASWeapon::OnStoppedFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

//special case: we dont need to declare the function in the .h ,unreal does it
void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_None);
	//DOREPLIFETIME_CONDITION(ASWeapon, TotalAmmo, COND_SkipOwner); // if you want to use a limited amount of ammo
	DOREPLIFETIME_CONDITION(ASWeapon, CurrentChamberAmmo, COND_None);
	DOREPLIFETIME_CONDITION(ASWeapon, bIsReloading, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ASWeapon, bIsFiring, COND_None);
}
