// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "TankAimingComponent.h"
#include "TankBarrel.h"
#include "TankTurret.h"


// Sets default values for this component's properties
UTankAimingComponent::UTankAimingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UTankAimingComponent::SetBarrelReference(UTankBarrel * BarrelToSet)
{
	Barrel = BarrelToSet;
}

void UTankAimingComponent::SetTurretReference(UTankTurret * TurretToSet)
{
	Turret = TurretToSet;
}

void UTankAimingComponent::AimAt(FVector HitLocation, float LaunchSpeed)
{
	if (!Barrel)
		return;

	if (!Turret)
		return;

	FVector OutLaunchVelocity;
	FVector StartLocation = Barrel->GetSocketLocation(FName("Projectile"));

	bool bHaveAimSolution = UGameplayStatics::SuggestProjectileVelocity(this, OutLaunchVelocity, StartLocation, HitLocation, LaunchSpeed, false, 0, 0, ESuggestProjVelocityTraceOption::DoNotTrace);

	// Calculate the out launch velocity 
	if (bHaveAimSolution)
	{
		OutLaunchVelocity.Normalize();

		MoveTurretTowards(OutLaunchVelocity);
		MoveBarrelTowards(OutLaunchVelocity);
	}
}

void UTankAimingComponent::MoveBarrelTowards(FVector Direction)
{
	// Work-out difference between current barrel rotation, and AimDirection
	FRotator BarrelRotation = Barrel->GetForwardVector().Rotation();
	FRotator WantedRotation = Direction.Rotation();

	FRotator DeltaRotator = WantedRotation - BarrelRotation;
	
	Barrel->Elevate(DeltaRotator.Pitch);
}

void UTankAimingComponent::MoveTurretTowards(FVector Direction)
{
	FRotator TurretRotation = Turret->GetForwardVector().Rotation();
	FRotator WantedRotation = Direction.Rotation();
	FRotator DeltaRotator = WantedRotation - TurretRotation;

	Turret->Rotate(DeltaRotator.Yaw);
}

