// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class USHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, ASWeapon*, CurrentWeapon, int, WeaponIndex);

UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);

	void MoveRight(float Value);

	void BeginCrouch();

	void EndCrouch();

	void Jump() override;

	void BeginZoom();

	void EndZoom();

	void NextWeapon();

	void ReloadWeapon();

	UFUNCTION(Server, Reliable, WithValidation, Category="Character")
	void EnableDisableWeapon(int32 WeaponIndex, bool bEnabled);

	UFUNCTION(Category="Character")
	void HandleHealthChanged(USHealthComponent* InHealthComponent, float Health, float HealthDelta,
							 const class UDamageType* DamageType, class AController* InstigatedBy,
							 AActor* DamageCauser);

	UFUNCTION(Category="Character")
	void DestroyWeapons();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartWeaponFire();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StopWeaponFire();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UCameraComponent* CameraComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Weapon", Replicated)
	TArray<ASWeapon*> Weapons;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", Replicated)
	ASWeapon* Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Weapon", Replicated)
	int32 SelectedWeaponIndex = -1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Weapon")
	TArray<TSubclassOf<ASWeapon>> WeaponClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	FName WeaponSocketName = FName("weapon_socket");

	UPROPERTY(VisibleAnywhere, BlueprintAssignable, Category = "Weapon")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	bool bWantsToZoom = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player")
	float ZoomedFOV = 65.0f;

	/** Default FOV set during play */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
	float DefaultFOV = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.0, ClampMax = 100.0f))
	float ZoomInterSpeed = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player", Replicated)
	USHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Player")
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Weapons")
	class USoundCue* WeaponChangeSound = nullptr;

	FTimerHandle TimerHandle_DieRagDollTimer;
};
