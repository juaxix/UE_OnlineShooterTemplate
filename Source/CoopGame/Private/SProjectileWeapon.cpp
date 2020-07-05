// Juan Belon - 2018
#include "../Public/SProjectileWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void ASProjectileWeapon::Fire()
{
	if (!CheckAmmo())
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerFire();
	}

	//get actor viewpoint
	AActor* owner = GetOwner();
	if (owner)
	{
		if (HasAuthority())
		{
			const FVector MuzzleLocation = SkeletalMeshComponent->GetSocketLocation(MuzzleSocketName);
			FVector EyeLocation;
			FRotator EyeRotation;
			owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* projectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnParams);
			projectile->SetReplicates(true);

			if (HasAuthority())
			{
				HitScanTrace.TraceFrom = MuzzleLocation;
				HitScanTrace.ImpactPoint = FVector::ZeroVector;
				HitScanTrace.SurfaceType = SurfaceType_Default;
			}
			CurrentChamberAmmo--;
		}

		PlayFireEffects();
		LastFireTime = GetWorld()->TimeSeconds;
	}
}
