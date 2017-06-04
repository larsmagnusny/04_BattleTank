// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "TankPlayerController.h"
#include "TrackedVehicle.h"


ATankPlayerController::ATankPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATankPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ATrackedVehicle* ControlledTank = GetControlledTank();

	if (ControlledTank)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tank: %s"), *ControlledTank->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Player is not posessing a tank"));
	}
}

void ATankPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimTowardsCrosshair();
}

void ATankPlayerController::AimTowardsCrosshair()
{
	if (!GetControlledTank())
		return;

	FVector HitLocation; // Out parameter
	if (GetSightRayHitLocation(HitLocation))
	{
		GetControlledTank()->AimAt(HitLocation);
	}
}

bool ATankPlayerController::GetSightRayHitLocation(FVector & OutHitLocation) const
{
	FHitResult Hit;
	// Get current dimentions of the screen
	int32 ViewportSizeX, ViewportSizeY;

	GetViewportSize(ViewportSizeX, ViewportSizeY);

	float ScreenX = ViewportSizeX*CrosshairXLocation;
	float ScreenY = ViewportSizeY*CrosshairYLocation;

	FVector2D ScreenLocation = FVector2D(ViewportSizeX*CrosshairXLocation, ViewportSizeY*CrosshairYLocation);

	if (GetHitResultAtScreenPosition(ScreenLocation, ECollisionChannel::ECC_Visibility, true, Hit))
	{
		OutHitLocation = Hit.ImpactPoint;
		return true;
	}

	return false;
}

ATrackedVehicle * ATankPlayerController::GetControlledTank() const
{
	return Cast<ATrackedVehicle>(GetPawn());
}

bool ATankPlayerController::GetLookDirection(FVector2D ScreenLocation, FVector& LookDirection) const
{
	FVector CameraWorldLocation; // To be discarded
	
	return DeprojectScreenPositionToWorld(ScreenLocation.X, ScreenLocation.Y, CameraWorldLocation, LookDirection);
}