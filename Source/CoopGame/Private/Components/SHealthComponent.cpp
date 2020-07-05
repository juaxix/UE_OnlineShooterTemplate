// Juan Belon - 2018
#include "../../Public/Components/SHealthComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameMode.h"
#include "../../Public/SGameMode.h"
#include "Engine/World.h"

USHealthComponent::USHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(true);
}

void USHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaultHealth);
	OnHealthChanged.Broadcast(this, Health, HealAmount, nullptr, nullptr, nullptr);
	SET_WARN_COLOR(COLOR_GREEN);
	UE_LOG(LogTemp, Log, TEXT("[+] Healing (%s). Health: %s"), *FString::SanitizeFloat(HealAmount),
		   *FString::SanitizeFloat(Health));
}


bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if (!ActorA || !ActorB)
	{
		return true;
	}

	USHealthComponent* HealthComponentA = Cast<USHealthComponent>(
		ActorA->GetComponentByClass(USHealthComponent::StaticClass()));

	USHealthComponent* HealthComponentB = Cast<USHealthComponent>(
		ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	if (!HealthComponentA || !HealthComponentB)
	{
		return true;
	}

	if (HealthComponentA == HealthComponentB)
	{
		return !HealthComponentA->bCanDamageMySelf;
	}

	return HealthComponentA->TeamNumber ==
		HealthComponentB->TeamNumber;
}


void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		AActor* owner = GetOwner();

		if (owner)
		{
			owner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}

	Health = DefaultHealth;
}

void USHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
											AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || IsFriendly(DamagedActor, DamageCauser) || IsDead()) return;
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);
	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
	if (Health <= 0.0f)
	{
		ASGameMode* SGM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (SGM)
		{
			SGM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}

	SET_WARN_COLOR(COLOR_YELLOW);
	UE_LOG(LogTemp, Log, TEXT("[-] Damaged: (%s) caused by %s. Health: %s"), *FString::SanitizeFloat(Damage),
		   *UKismetSystemLibrary::GetDisplayName(DamageCauser), *FString::SanitizeFloat(Health));
}


void USHealthComponent::RepOn_HealthChanged(float OldHealth)
{
	OnHealthChanged.Broadcast(this, Health, Health - OldHealth, nullptr, nullptr, nullptr);
}

void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USHealthComponent, Health);
}
