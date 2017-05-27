// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "TankBarrel.h"

UTankBarrel::UTankBarrel()
{

}

void UTankBarrel::Elevate(float RelativeSpeed)
{
	// Move the barrel the right amount this frame
	// Given a max elevation speed, and the frame time
	RelativeSpeed = FMath::Clamp<float>(RelativeSpeed, -1, 1);
	float ElevationChange = RelativeSpeed * MaxDegreesPerSecond * GetWorld()->GetDeltaSeconds();

	float RawNewElevation = RelativeRotation.Pitch + ElevationChange;

	RawNewElevation = FMath::ClampAngle(RawNewElevation, minDegreesElevation, maxDegreesElevation);

	SetRelativeRotation(FRotator(RawNewElevation, 0, 0));
}