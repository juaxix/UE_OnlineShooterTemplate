// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SExplosiveBarrel.generated.h"

class USHealthComponent;
class UStaticMeshComponent;
class URadialForceComponent;
class UParticleSystem;
//class UMaterial;

UCLASS()
class COOPGAME_API ASExplosiveBarrel : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASExplosiveBarrel();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(Category="Props")
	void HandleHealthChanged(USHealthComponent* InHealthComponent, float Health, float HealthDelta,
							 const class UDamageType* DamageType, class AController* InstigatedBy,
							 AActor* DamageCauser);

	UFUNCTION(Category="Props")
	void OnRep_Exploded();
public:
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components", Replicated)
	USHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	URadialForceComponent* RadialForceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrel", ReplicatedUsing=OnRep_Exploded)
	bool bExploded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	float ExplosionImpulse = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	UParticleSystem* ExplosionEffect = nullptr;

	//Material to replace when the barrel explodes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	UMaterialInterface* BarrelExplodedMaterial = nullptr;
};
