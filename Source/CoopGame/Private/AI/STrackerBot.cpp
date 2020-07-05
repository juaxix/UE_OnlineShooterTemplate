// Juan Belon - 2018

#include "../../Public/AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "../../Public/Components/SHealthComponent.h"
#include "../../Public/AI/STrackerBot.h"
#include "../../Public/SCharacter.h"
#include "Sound/SoundCue.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	AActor::SetReplicateMovement(true);
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetSimulatePhysics(true);
	RootComponent = MeshComponent;

	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("ShpereComponent"));
	SphereComponent->SetSphereRadius(200.0f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComponent->SetupAttachment(RootComponent);
}

bool ASTrackerBot::CheckMaterialInstance()
{
	if (MaterialInstance == nullptr)
	{
		MaterialInstance = MeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(
			0, MeshComponent->GetMaterial(0));
	}

	return MaterialInstance != nullptr;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	if (!HealthComponent->OnHealthChanged.IsBound())
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HealthChanged);
	}

	if (HasAuthority())
	{
		NextPathPoint = GetNextPathPoint();
		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearbyBots, 0.5f,
										true);
		MeshBaseColor = FLinearColor(
			FMath::FRandRange(0.0f, 1.0f),
			FMath::FRandRange(0.0f, 1.0f),
			FMath::FRandRange(0.0f, 1.0f),
			1.0f
		);
		OnRep_MeshBaseColor();
	}

	RollingAudioComponent = Cast<UAudioComponent>(GetComponentByClass(UAudioComponent::StaticClass()));
}


bool ASTrackerBot::UpdateBotInServer_Validate()
{
	return true;
}

void ASTrackerBot::UpdateBotInServer_Implementation()
{
	if (FVector::Distance(GetActorLocation(), NextPathPoint) <= RequiredDistanceToTarget)
	{
		NextPathPoint = GetNextPathPoint();
		//DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached!", NULL, FColor::White, 1.2f, true);
	}
	else
	{
		FVector ForceDirection = NextPathPoint - GetActorLocation();
		ForceDirection.Normalize();
		ForceDirection *= MovementForce;

		MeshComponent->AddForce(ForceDirection, NAME_None, bUseVelocityChange);
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32,
								  FColor::Yellow, false, 0.0f, 1.0f);
	}
	DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 1.0f);
}

void ASTrackerBot::RefresNextPathPoint()
{
	if (HasAuthority())
	{
		NextPathPoint = GetNextPathPoint();
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	USHealthComponent* TargetHealthComponent = Target != nullptr
												   ? Cast<USHealthComponent>(
													   Target->GetComponentByClass(USHealthComponent::StaticClass()))
												   : nullptr;

	if (Target == nullptr || TargetHealthComponent == nullptr || TargetHealthComponent->IsDead())
	{
		APawn* BestPawn = nullptr;
		const FVector& Location = GetActorLocation();
		float BestDistance = TNumericLimits<float>::Max();
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			APawn* pawn = It->Get();
			if (!pawn || USHealthComponent::IsFriendly(pawn, this))
			{
				continue;
			}
			TargetHealthComponent = Cast<USHealthComponent
			>(pawn->GetComponentByClass(USHealthComponent::StaticClass()));
			if (TargetHealthComponent && !TargetHealthComponent->IsDead())
			{
				float Distance = FVector::Distance(Location, pawn->GetActorLocation());
				if (Distance < BestDistance)
				{
					BestPawn = pawn;
					BestDistance = Distance;
				}
			}
		}
		Target = BestPawn;
	}
	if (Target != nullptr)
	{
		UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(
			this, //UObject* WorldContextObject
			GetActorLocation(), //const FVector& PathStart
			Target //AActor* GoalActor
			//float TetherDistance = 50.f,
			//AActor* PathfindingContext = NULL,
			//TSubclassOf<UNavigationQueryFilter> FilterClass = NULL
		);

		GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath); //avoid unnecessary pathing
		GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::RefresNextPathPoint, 5.5f, true);


		if (NavPath && NavPath->PathPoints.Num() > 1)
		{
			return NavPath->PathPoints[1];
		}
	}
	return GetActorLocation();
}


void ASTrackerBot::HealthChanged(USHealthComponent* InHealthComponent, float Health, float HealthDelta,
								 const class UDamageType* DamageType, class AController* InstigatedBy,
								 AActor* DamageCauser)
{
	if (CheckMaterialInstance())
	{
		MaterialInstance->SetScalarParameterValue(FName("LastTimeDamageTaken"), GetWorld()->TimeSeconds);
	}

	// Explode on hitpoint == 0
	if (Health <= 0.0f)
	{
		SelfDestruct();
	}

	UE_LOG(LogTemp, Log, TEXT("Current Health : %s of %s"), *FString::SanitizeFloat(Health), *GetName());
}


// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority() && !bExploded)
	{
		UpdateBotInServer();
	}

	//servers and clients: update rolling sound volume depending on the speed
	if (RollingAudioComponent)
	{
		RollingAudioComponent->SetVolumeMultiplier(
			UKismetMathLibrary::MapRangeClamped(GetVelocity().Size(), 10.0f, 1000.0f, 0.1f, 2.0f)
		);
	}
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bSelfDestructionStarted || bExploded)
	{
		return;
	}

	ASCharacter* Player = Cast<ASCharacter>(OtherActor);
	if (Player && !USHealthComponent::IsFriendly(this, OtherActor))
	{
		bSelfDestructionStarted = true;
		HealthComponent->bCanDamageMySelf = true;
		if (HasAuthority())
		{
			//starts self destruction sequence
			GetWorldTimerManager().SetTimer(TimerHandle_SelfDamageStart, this, &ASTrackerBot::DamageSelf,
											SelfDamageInterval, true, 0.0f);
		}

		if (SelfDestructSound)
		{
			//USoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true)
			UGameplayStatics::SpawnSoundAttached(
				SelfDestructSound, RootComponent, NAME_None, GetActorLocation(),
				EAttachLocation::KeepRelativeOffset, true, 1.0f, 1.0f, 0.0f, 
				nullptr,nullptr, true);
		}
	}
}

void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;
	DrawDebugString(GetWorld(), GetActorLocation(), "Exploding!!", NULL, FColor::Red, 1.2f, true);
	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadiusForDamage, 12, FColor::Red, false, 2.0f);
	//FX
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
	}

	MeshComponent->SetVisibility(false, true);
	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (HasAuthority())
	{
		//Explosion Damage in radius
		//Calculate the damage increased by the power level
		const float ActualRadialDamage = ExplosionRadialDamage + (ExplosionRadialDamage * PowerLevel);
		UGameplayStatics::ApplyRadialDamage(this, ActualRadialDamage, GetActorLocation(), ExplosionRadiusForDamage,
											nullptr, TArray<AActor*>{this}, this, GetInstigatorController(), true);
		GetWorldTimerManager().ClearTimer(TimerHandle_SelfDamageStart);

		SetLifeSpan(2.0f);
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20.0f /**every tick of the timer ,hits for damage*/, GetInstigatorController(),
								  this, NULL);
}

void ASTrackerBot::OnCheckNearbyBots()
{
	//distance to check for nearby bots
	const float Radius = 666;

	//Create temporary collision shape for overlaps
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(Radius);

	//Only find pawns (players and AI bots)
	FCollisionObjectQueryParams QueryParams;
	//Our tracker bot's mesh component is set to Physics Body in Blueprint (default profile of physics simulated actors)
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollisionShape);
	//DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::Cyan, false, 1.0f);

	//loop over the results using a range based foreach loop
	int32 NrOfBots = 0;
	for (FOverlapResult Result : Overlaps)
	{
		//Check if we overlapped with another tracker bot (ignoring players and other bot types)
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());
		//Ignore this trackerbot instance
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}
	const int MaxPowerLevel = 4;
	//Clamp between min = 0 and max = 4
	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);

	//Update the material color
	if (CheckMaterialInstance())
	{
		//Convert to a float between 0 and 1 just like an "alpha" value of a texture. Nw the material can be set up without
		//having to know the max power level which can be tweaked many times by gameplay decissions (would mean we need to
		//keep 2 places up to date)
		float Alpha = PowerLevel / (float)MaxPowerLevel; //we need to convert float to int to divide with correct values
		MaterialInstance->SetScalarParameterValue(FName("PowerLevelAlpha"), Alpha);
	}

	//Draw on the bot location
	//DrawDebugString(GetWorld(), FVector::ZeroVector, FString::FromInt(PowerLevel), this, FColor::White, 1.0f, true);
}

void ASTrackerBot::OnRep_MeshBaseColor()
{
	if (CheckMaterialInstance())
	{
		MaterialInstance->SetVectorParameterValue(FName("MainColor"), MeshBaseColor);
	}
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASTrackerBot, MeshBaseColor);
	DOREPLIFETIME(ASTrackerBot, Target);
}
