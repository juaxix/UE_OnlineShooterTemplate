// Juan Belon - 2018

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPickUp.generated.h"

class USphereComponent;
class UDecalComponent;
class ASPowerUp;

UCLASS()
class COOPGAME_API ASPickUp : public AActor
{
	GENERATED_BODY()

public:

	ASPickUp();

protected:
	virtual void BeginPlay() override;

	//Loop spawning after pickup
	UFUNCTION(Category="PickUp")
	void Respawn();

public:
	//virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:

	UPROPERTY(VisibleAnywhere, Category="Components")
	USphereComponent* SphereComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UDecalComponent* DecalComponent = nullptr;

	UPROPERTY(EditInstanceOnly, Category="PickUp")
	TSubclassOf<ASPowerUp> PowerUpClass = nullptr;

	UPROPERTY(VisibleAnywhere, Category="PowerUp")
	ASPowerUp* CurrentPowerUp = nullptr;

	UPROPERTY(EditAnywhere, Category="PickUp")
	float PickedCoolDown = 10.0f;

	FTimerHandle TimerHandle_PickupRespawn;

	UPROPERTY(EditDefaultsOnly, Category = "PickUp")
	FLinearColor DecalColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "PickUp", meta = (AllowPrivateAccess = true))
	class USoundCue* PickUpSound = nullptr;
};
