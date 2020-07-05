// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHealthComponent.generated.h"

//On Health change delegate declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature, USHealthComponent*, HealthComponent, float,
											 Health, float, HealthDelta, const class UDamageType*, DamageType,
											 class AController*, InstigatedBy, AActor*, DamageCauser);


UCLASS(ClassGroup=(COOP), meta=(BlueprintSpawnableComponent))
class COOPGAME_API USHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USHealthComponent();

	UFUNCTION(BlueprintCallable, Category="Health Component")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintPure, Category="Health Component")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure,Category="Health Component")
	bool IsDead() const { return Health <= 0.0f; }

	UFUNCTION(BlueprintPure, Category="Health Component")
	static bool IsFriendly(AActor* ActorA, AActor* ActorB);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Category="Health Component")
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
							 class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(Category="Health Component")
	void RepOn_HealthChanged(float OldHealth);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Health Component", ReplicatedUsing=RepOn_HealthChanged,
		meta=(Tooltip="Current Health, it is assigned to DefaultHealth at the begin"))
	float Health = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health Component")
	float DefaultHealth = 100.0f;

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health Component")
	uint8 TeamNumber = 0; //replicated only if the team will change

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Component")
	bool bCanDamageMySelf = false;
};
