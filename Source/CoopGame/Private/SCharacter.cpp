// Juan Belon - 2018

#include "../Public/SCharacter.h"
#include "../Public/SWeapon.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShake.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "../Public/Components/SHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "CoopGame.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "Sound/SoundCue.h"
// Sets default values
ASCharacter::ASCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->SetupAttachment(RootComponent);
	ACharacter::GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->GravityScale = 1.5f; //suck to the ground quicker
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && WeaponClasses.Num() > 0)
	{
		for (int32 i = 0; i < WeaponClasses.Num(); ++i)
		{
			FActorSpawnParameters params;
			params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			params.Owner = this;
			params.Instigator = Instigator;
			ASWeapon* NewWeapon = GetWorld()->SpawnActor<ASWeapon>(
				WeaponClasses[i],
				FVector::ZeroVector, FRotator::ZeroRotator, params
			);
			NewWeapon->SetReplicates(true);
			Weapons.AddUnique(NewWeapon);
			FAttachmentTransformRules attachrules(
				EAttachmentRule::KeepRelative,
				EAttachmentRule::KeepRelative,
				EAttachmentRule::SnapToTarget,
				true
			);

			Weapons[i]->AttachToComponent(GetMesh(), attachrules, WeaponSocketName);
			if (i == 0)
			{
				EnableDisableWeapon(i, true);
			}
			else
			{
				EnableDisableWeapon(i, false);
			}
		}
	}

	DefaultFOV = CameraComponent->FieldOfView;
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASCharacter::HandleHealthChanged);
}


void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void ASCharacter::BeginCrouch()
{
	if (!bWasJumping)
	{
		Crouch();
	}
}

void ASCharacter::EndCrouch()
{
	UnCrouch();
}

void ASCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch(false);
	}

	Super::Jump();
}

void ASCharacter::BeginZoom()
{
	bWantsToZoom = true;
}

void ASCharacter::EndZoom()
{
	bWantsToZoom = false;
}

void ASCharacter::NextWeapon()
{
	if (Weapon)
	{
		Weapon->StopFire();
	}

	int32 NextWeaponIndex = SelectedWeaponIndex + 1;
	if (NextWeaponIndex >= Weapons.Num())
	{
		NextWeaponIndex = 0;
	}

	EnableDisableWeapon(NextWeaponIndex, true);
}

void ASCharacter::ReloadWeapon()
{
	if (!HealthComponent->IsDead() && Weapon)
	{
		Weapon->Reload();
	}
}

void ASCharacter::EnableDisableWeapon_Implementation(int32 WeaponIndex, bool bEnabled)
{
	/*UE_LOG(LogTemp, Log,
		TEXT("E/D Weapon %d: %s for %s"),
		WeaponIndex, bEnabled ? TEXT("YES") : TEXT("NO"),
		*GetName()
	);*/

	if (Weapons.IsValidIndex(WeaponIndex))
	{
		ASWeapon* new_weapon = Weapons[WeaponIndex];
		if (!new_weapon)
		{
			return;
		}

		if (bEnabled && WeaponIndex != SelectedWeaponIndex)
		{
			EnableDisableWeapon(SelectedWeaponIndex, false);
			Weapon = new_weapon;
			SelectedWeaponIndex = WeaponIndex;
			OnWeaponChanged.Broadcast(Weapon, WeaponIndex);
		}
		new_weapon->SetActorHiddenInGame(!bEnabled);
		new_weapon->SetActorEnableCollision(!bEnabled);

		if (WeaponChangeSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WeaponChangeSound, GetActorLocation());
		}
	}
}

bool ASCharacter::EnableDisableWeapon_Validate(int32 WeaponIndex, bool bEnabled)
{
	return true;
}

void ASCharacter::HandleHealthChanged(USHealthComponent* InHealthComponent, float Health, float HealthDelta,
									  const class UDamageType* DamageType, class AController* InstigatedBy,
									  AActor* DamageCauser)
{
	if (Health <= 0.0f && !bIsDead)
	{
		//Die
		bIsDead = true;
		GetMovementComponent()->StopMovementImmediately();
		DetachFromControllerPendingDestroy();
		SetLifeSpan(10.0f); //after 10 sec this pawn will get destroyed
		StopWeaponFire();
		DestroyWeapons();
		GetWorldTimerManager().SetTimer(TimerHandle_DieRagDollTimer, [this]()
		{
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
			GetMesh()->SetSimulatePhysics(true);
			GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}, 0.0f, false, 3.0f);
	}
}


void ASCharacter::DestroyWeapons()
{
	for (ASWeapon* weapon : Weapons)
	{
		if (weapon)
		{
			weapon->Destroy(true, false);
		}
	}

	Weapons.Empty();
	Weapon = nullptr;
}

void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	const float CurrentFOV = FMath::FInterpTo(CameraComponent->FieldOfView, TargetFOV, DeltaTime, ZoomInterSpeed);
	CameraComponent->SetFieldOfView(CurrentFOV);
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::Jump);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartWeaponFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopWeaponFire);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("NextWeapon", IE_Pressed, this, &ASCharacter::NextWeapon);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::ReloadWeapon);
}

FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComponent)
	{
		return CameraComponent->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}

void ASCharacter::StartWeaponFire()
{
	if (bIsDead)
	{
		return;
	}

	if (Weapon)
	{
		Weapon->StartFire();
	}
}

void ASCharacter::StopWeaponFire()
{
	if (Weapon)
	{
		Weapon->StopFire();
	}
}

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASCharacter, Weapons);
	DOREPLIFETIME(ASCharacter, Weapon);
	DOREPLIFETIME(ASCharacter, SelectedWeaponIndex);
	DOREPLIFETIME(ASCharacter, HealthComponent);
}
