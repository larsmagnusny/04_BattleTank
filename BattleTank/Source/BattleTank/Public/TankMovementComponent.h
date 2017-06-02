// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "TankMovementComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLETANK_API UTankMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTankMovementComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	void AddThrust(float AxisValue);

	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<FName> Bones;

	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	
	FVector LastImpact = FVector(0, 0, 0);

	UPROPERTY(EditDefaultsOnly, Category = "Angular Acceleration")
	float AngAccel = 10.f;

	UPROPERTY(BlueprintReadOnly, Category = "Angular Velocity")
	float Velocity = 0.f;
};
