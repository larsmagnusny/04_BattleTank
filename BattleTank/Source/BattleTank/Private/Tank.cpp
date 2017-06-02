// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "Tank.h"
#include "TankAimingComponent.h"
#include "TankMovementComponent.h"
#include "TankBarrel.h"
#include "TankTurret.h"
#include "Projectile.h"


// Sets default values
ATank::ATank()
{
	TankAimingComponent = CreateDefaultSubobject<UTankAimingComponent>(FName("Aiming Component"));
	TankMovementComponent = CreateDefaultSubobject <UTankMovementComponent>(FName("Movement Component"));
	// Create the colliders for the track
	/*for (int i = 1; i <= 95; i++)
	{
		FString name = "J" + FString::FromInt(i);
		UCapsuleComponent* Comp = CreateDefaultSubobject<UCapsuleComponent>(FName(*name));

		TrackColliders.Add(Comp);
	}*/
}

// Called when the game starts or when spawned
void ATank::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called to bind functionality to input
void ATank::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

}

void ATank::SetBarrelReference(UTankBarrel * BarrelToSet)
{
	if (TankAimingComponent)
		TankAimingComponent->SetBarrelReference(BarrelToSet);
	Barrel = BarrelToSet;
}

void ATank::SetTurretReference(UTankTurret * TurretToSet)
{
	if (TankAimingComponent)
		TankAimingComponent->SetTurretReference(TurretToSet);
}

void ATank::SetSkeletalMeshComponentReference(USkeletalMeshComponent * Component, TArray<FName> Bones)
{
	TankMovementComponent->SkeletalMeshComponent = Component;
	TankMovementComponent->Bones = Bones;
}

void ATank::AddThrust(float AxisValue)
{
	TankMovementComponent->AddThrust(AxisValue);
}

void ATank::Fire()
{

	bool isReloaded = (FPlatformTime::Seconds() - LastFireTime) > ReloadTimeInSeconds;
	if (Barrel && isReloaded)
	{

		// Spawn a projectile at the socket location on the barrel
		FActorSpawnParameters p;
		AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileBlueprint, Barrel->GetSocketLocation(FName("Projectile")), Barrel->GetSocketRotation(FName("Projectile")));

		Projectile->LaunchProjectile(LaunchSpeed);

		LastFireTime = FPlatformTime::Seconds();
	}
}

void ATank::AimAt(FVector HitLocation)
{
	if (TankAimingComponent)
		TankAimingComponent->AimAt(HitLocation, LaunchSpeed);
}

