// Juan Belon - 2018

#include "../Public/SExplosiveBarrel.h"
#include "../Public/Components/SHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ASExplosiveBarrel::ASExplosiveBarrel()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComponent;

	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));

	RadialForceComponent = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComponent"));
	RadialForceComponent->SetupAttachment(MeshComponent);
	RadialForceComponent->Radius = 250;
	RadialForceComponent->bImpulseVelChange = true;
	RadialForceComponent->bAutoActivate = false; //prevent component from ticking and use only FireImpulse instead
	RadialForceComponent->bIgnoreOwningActor = true; //ignore self

	ExplosionImpulse = 400.0f;
	SetReplicates(true);
	AActor::SetReplicateMovement(true);
}

void ASExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASExplosiveBarrel::HandleHealthChanged);
}

void ASExplosiveBarrel::HandleHealthChanged(USHealthComponent* InHealthComponent, float Health, float HealthDelta,
											const class UDamageType* DamageType, class AController* InstigatedBy,
											AActor* DamageCauser)
{
	if (bExploded)
	{
		return; //already done
	}

	if (Health <= 0.0f)
	{
		bExploded = true;
		OnRep_Exploded();

		//Boost the barrel upwards
		FVector BoostIntensity = FVector::UpVector * ExplosionImpulse;
		MeshComponent->AddImpulse(BoostIntensity, NAME_None, true);

		//Play FX and change self material to black
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

		//Override material on mesh with blackened version
		MeshComponent->SetMaterial(0, BarrelExplodedMaterial);

		//Blast away nearby physics actors
		RadialForceComponent->FireImpulse();

		//@todo: apply radial damage
	}
}

void ASExplosiveBarrel::OnRep_Exploded()
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	MeshComponent->SetMaterial(0, BarrelExplodedMaterial);
}


void ASExplosiveBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASExplosiveBarrel, HealthComponent);
	DOREPLIFETIME(ASExplosiveBarrel, bExploded);
}
