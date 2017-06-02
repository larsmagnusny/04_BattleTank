// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "TankMovementComponent.h"


// Sets default values for this component's properties
UTankMovementComponent::UTankMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
	SetTickGroup(ETickingGroup::TG_DuringPhysics);
	// ...
}


// Called when the game starts
void UTankMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	
}


// Called every frame
void UTankMovementComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	if (!SkeletalMeshComponent)
		return;

	Velocity = SkeletalMeshComponent->GetPhysicsLinearVelocity().Size();

	if (Velocity < 1.0f)
	{
		for (int i = 0; i < Bones.Num(); i++)
			SkeletalMeshComponent->SetPhysicsAngularVelocity(FVector(0, 0, 0), false, Bones[i]);
	}
}

void UTankMovementComponent::AddThrust(float AxisValue)
{
	if(Velocity < 2220)
		SkeletalMeshComponent->AddImpulse(GetOwner()->GetActorForwardVector()*1000*AxisValue*GetWorld()->GetDeltaSeconds(), NAME_None, true);
}

