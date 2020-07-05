// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class UStaticMeshComponent;
class USHealthComponent;
class UParticleSystem;
class USphereComponent;
class UAudioComponent;
class USoundCue;

UCLASS()
class COOPGAME_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:

	ASTrackerBot();

private:
	bool CheckMaterialInstance();
protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void UpdateBotInServer();

	UFUNCTION(Category="Tracker Bot")
	void RefresNextPathPoint();

	UFUNCTION(Category="Tracker Bot")
	FVector GetNextPathPoint();

	UFUNCTION(Category="Tracker Bot")
	void HealthChanged(USHealthComponent* InHealthComponent, float Health, float HealthDelta,
					   const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(Category="Tracker Bot")
	void SelfDestruct();

	UFUNCTION(Category="Tracker Bot")
	void DamageSelf();

	UFUNCTION(Category="Tracker Bot")
	void OnCheckNearbyBots();

	UFUNCTION(Category="Tracker Bot")
	void OnRep_MeshBaseColor();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* MeshComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USphereComponent* SphereComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TrackerBot", Replicated)
	APawn* Target = nullptr;

	//next point in navigation path
	FVector NextPathPoint = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float MovementForce = 1000;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	bool bUseVelocityChange = false;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	float RequiredDistanceToTarget = 100;

	UPROPERTY(EditDefaultsOnly, Category="TrackerBot", meta=(ToolTip="Damage to be applied in the damage radius"))
	float ExplosionRadialDamage = 66.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot", meta = (ToolTip =
		"Damage radius to be applied around this actor"))
	float ExplosionRadiusForDamage = 333.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot", meta = (ToolTip =
		"Time to damage itself before explode once the bot touches a target"))
	float SelfDamageInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category="TrackerBot")
	UParticleSystem* ExplosionEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="TrackerBot", ReplicatedUsing=OnRep_MeshBaseColor)
	FLinearColor MeshBaseColor = FLinearColor::White;

	//the power boost of the bot, affects damaged caused to enemies and color of the bot (range: 1 to 4)
	int32 PowerLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TrackerBot")
	bool bExploded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TrackerBot")
	bool bSelfDestructionStarted = false;

	///Dynamic material to pulse on damage
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UMaterialInstanceDynamic* MaterialInstance = nullptr;

	//Optional component to make sound rolling/walking
	//should be added and configured on blueprints inheriting from this class
	UAudioComponent* RollingAudioComponent = nullptr;

	FTimerHandle TimerHandle_SelfDamageStart;
	FTimerHandle TimerHandle_RefreshPath;

	UPROPERTY(EditDefaultsOnly, Category="TrackerBot")
	USoundCue* SelfDestructSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "TrackerBot")
	USoundCue* ExplosionSound = nullptr;
};
