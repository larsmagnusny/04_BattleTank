// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "Tank.generated.h"

class UTankAimingComponent;
class UTankBarrel;
class UTankTurret;
class AProjectile;

UCLASS()
class BATTLETANK_API ATank : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATank();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Set the barrel reference")
	void SetBarrelReference(UTankBarrel* BarrelToSet);

	UFUNCTION(BlueprintCallable, Category = "Set the turret reference")
	void SetTurretReference(UTankTurret* TurretToSet);

	UFUNCTION(BlueprintCallable, Category = "Fire the weapon")
	void Fire();

	void AimAt(FVector HitLocation);
	
	UPROPERTY(EditAnywhere, Category = "Firing")
	float LaunchSpeed = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Set the projectile class")
	TSubclassOf<AProjectile> ProjectileBlueprint = nullptr;
protected:
	UTankAimingComponent* TankAimingComponent = nullptr;
private:
	// Local barrel reference for spawning projectile
	UTankBarrel* Barrel = nullptr;
};
