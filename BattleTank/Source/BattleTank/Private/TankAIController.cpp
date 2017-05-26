// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "../Public/TankAIController.h"

ATankAIController::ATankAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATankAIController::BeginPlay()
{
	Super::BeginPlay();

	ATank* Tank = GetControlledTank();

	if (Tank)
	{
		UE_LOG(LogTemp, Warning, TEXT("AI is posessing tank: %s"), *Tank->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AI is not posessing a tank"));
	}
}

void ATankAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

ATank * ATankAIController::GetControlledTank()
{
	return Cast<ATank>(GetPawn());
}
