// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;
class UCameraShake;
class USoundCue;

///Contains information of a single weapon line trace
///if unreal server sees no important changes in the struct it will be not sync
USTRUCT(BlueprintType)
struct FHitScanTrace
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	FVector_NetQuantize TraceFrom;

	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	FVector_NetQuantize ImpactPoint;
};

UCLASS(Blueprintable,BlueprintType)
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()

public:

	ASWeapon();

protected:
	virtual void BeginPlay() override;
	virtual void Fire();

	UFUNCTION(Server, Reliable, WithValidation, Category="Weapons")
	void ServerFire();

	virtual bool CheckAmmo();

public:
	UFUNCTION(BlueprintCallable, Category="Weapons")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual void PlayFireEffects(FVector EndPoint = FVector::ZeroVector, FRotator EndRotation = FRotator::ZeroRotator,
								 EPhysicalSurface SurfaceType = EPhysicalSurface::SurfaceType_Default);


	UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual bool HasAmmo() const
	{
		return CurrentChamberAmmo > 0
		//* @todo: if you want to use a limited ammo instead of infinite reloads
		//&& TotalAmmo > 0
		;
	}

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual bool CanReloadAmmo() const
	{
		return
		  //* @todo: if you want to use a limited ammo instead of infinite reloads ucomment totalammo
		  // TotalAmmo > 0 &&
		  CurrentChamberAmmo < MaxChamberAmmo;
	}

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual void Reload();

	//notification from server to play effects
	UFUNCTION(Category="Weapons")
	virtual void OnRep_HitScanTrace(FHitScanTrace OldHitScanTrace);

	UFUNCTION(Category="Weapons")
	virtual void OnRep_IsFiring();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual void OnStartedFire();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	virtual void OnStoppedFire();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Sounds")
	USoundCue* StartFireSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Sounds")
	USoundCue* HitFireSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Sounds")
	USoundCue* HitFireEnemySound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Sounds")
	USoundCue* ReloadSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons|Sounds")
	bool bPlayStartSoundPerProjectile = false;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess=true))
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<UDamageType> DamageType = nullptr;
	//it's a subclass instead of a pointer because we never have to change variables in a damage type

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName MuzzleSocketName = FName("MuzzleFlashSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName TracerParamName = FName("BeamEnd");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ToolTip =
		"Default impact particle effect"))
	UParticleSystem* ImpactEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ToolTip = "Flesh impact particle effect"
	))
	UParticleSystem* FleshImpactEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<UCameraShake> FireCameraShake = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BaseDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta=(ClampMin=0.0f))
	float BulletSpread = 1.0f;

	FTimerHandle TimerHandle_TimeBetweenShots;
	FTimerHandle TimerHandle_Reloading;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ToolTip = "Number of bullets per minute fired"))
	float FireRate = 600.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	float LastFireTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	float TimeBetweenShots = 60.0f / FireRate;

	/**
	 * @todo: if you want to use a limited ammo instead of infinite reloads
	 *UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ToolTip =
		"Number of bullets available to shoot"))
	int32 TotalAmmo = 1000;*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon", meta = (ToolTip =
		"Max number of bullets in the chamber"))
	int32 MaxChamberAmmo = 10;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ToolTip =
		"Current number of bullets in the chamber"))
	int32 CurrentChamberAmmo = 10;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ToolTip =
		"Flag to know when the weapon is in reloading time"))
	bool bIsReloading = false;

	UPROPERTY(ReplicatedUsing= OnRep_IsFiring, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ToolTip
		= "Flag to know when the weapon is firing"))
	bool bIsFiring = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ToolTip =
		"Time in seconds to reload the weapon"))
	float TimeToReload = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD")
	UTexture2D* WeaponIcon;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_HitScanTrace, Category = "Weapon")
	FHitScanTrace HitScanTrace;
};
