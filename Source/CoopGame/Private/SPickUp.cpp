// Juan Belon - 2018

#include "../Public/SPickUp.h"
#include "../Public/SPowerUp.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ASPickUp::ASPickUp()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetSphereRadius(75.0f);
	RootComponent = SphereComponent;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	DecalComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	DecalComponent->DecalSize = FVector(64.0f, 75.0f, 75.0f);
	DecalComponent->SetupAttachment(RootComponent);

	SetReplicates(true);
}


void ASPickUp::BeginPlay()
{
	Super::BeginPlay();

	Respawn();
}

void ASPickUp::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PowerUpClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Select a PowerUp class in this PickUp: %s"), *GetName());
		return;
	}
	if (CurrentPowerUp != nullptr)
	{
		return;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentPowerUp = GetWorld()->SpawnActor<ASPowerUp>(PowerUpClass, GetTransform(), SpawnParams);
}

void ASPickUp::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if (!HasAuthority() || !CurrentPowerUp ||
		!OtherActor || !OtherActor->IsA<ACharacter>())
	{
		return;
	}

	if (CurrentPowerUp != nullptr)
	{
		CurrentPowerUp->ActivatePowerUp(Cast<ACharacter>(OtherActor));
		CurrentPowerUp = nullptr;
		if (PickUpSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickUpSound, GetActorLocation());
		}

		GetWorldTimerManager().SetTimer(TimerHandle_PickupRespawn, this, &ASPickUp::Respawn, PickedCoolDown);
	}
}
