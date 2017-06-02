// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Runtime/Engine/Classes/PhysicsEngine/PhysicsConstraintComponent.h"
#include "Carriage.generated.h"

UCLASS()
class BATTLETANK_API ACarriage : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACarriage();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	UStaticMeshComponent* GetBackCoupler();
	UStaticMeshComponent* GetFrontCoupler();

	UPhysicsConstraintComponent* PC_Back_Coupler = nullptr;
	UStaticMeshComponent* F_L03 = nullptr;
	UPhysicsConstraintComponent* PC_F_L03 = nullptr;
	UStaticMeshComponent* F_L03_Axel = nullptr;
	UPhysicsConstraintComponent* PC_F_L03_Axel = nullptr;
	UStaticMeshComponent* F_R03 = nullptr;
	UPhysicsConstraintComponent* PC_F_R03 = nullptr;
	UStaticMeshComponent* F_R03_Axel = nullptr;
	UPhysicsConstraintComponent* PC_F_R03_Axel = nullptr;
	UPhysicsConstraintComponent* PC_F_L02_Axel = nullptr;
	UStaticMeshComponent* F_L02_Axel = nullptr;
	UPhysicsConstraintComponent* PC_F_L02 = nullptr;
	UPhysicsConstraintComponent* PC_F_R02_Axel = nullptr;
	UStaticMeshComponent* F_L02 = nullptr;
	UStaticMeshComponent* F_R02_Axel = nullptr;
	UPhysicsConstraintComponent* PC_F_R02 = nullptr;
	UStaticMeshComponent* F_R01 = nullptr;
	UPhysicsConstraintComponent* PC_F_L01_Axel = nullptr;
	UStaticMeshComponent* F_L01_Axel = nullptr;
	UPhysicsConstraintComponent* PC_F_R01_Axel = nullptr;
	UStaticMeshComponent* F_R01_Axel = nullptr;
	UPhysicsConstraintComponent* PC_L_R01 = nullptr;
	UStaticMeshComponent* F_L01 = nullptr;
	UPhysicsConstraintComponent* PC_F_R01 = nullptr;
	UStaticMeshComponent* F_R02 = nullptr;
	UPhysicsConstraintComponent* PC_Front_Coupler = nullptr;
	UStaticMeshComponent* Front_Coupler = nullptr;
	UStaticMeshComponent* Back_Coupler = nullptr;
	UPhysicsConstraintComponent* PC_B_L03 = nullptr;
	UPhysicsConstraintComponent* PC_L03_Axel = nullptr;
	UStaticMeshComponent* B_L03_Axel = nullptr;
	UPhysicsConstraintComponent* PC_B_R03 = nullptr;
	UPhysicsConstraintComponent* PC_B_R03_Axel = nullptr;
	UStaticMeshComponent* B_R03_Axel = nullptr;
	UPhysicsConstraintComponent* PC_B_L02 = nullptr;
	UPhysicsConstraintComponent* PC_B_L02_Axel = nullptr;
	UStaticMeshComponent* B_L02_Axel = nullptr;
	UPhysicsConstraintComponent* PC_B_R02 = nullptr;
	UPhysicsConstraintComponent* PC_B_R02_Axel = nullptr;
	UStaticMeshComponent* B_R02_Axel = nullptr;
	UPhysicsConstraintComponent* PC_B_L01 = nullptr;
	UPhysicsConstraintComponent* PC_B_L01_Axel = nullptr;
	UStaticMeshComponent* B_L01_Axel = nullptr;
	UPhysicsConstraintComponent* PC_B_R01 = nullptr;
	UPhysicsConstraintComponent* PC_B_R01_Axel = nullptr;
	UStaticMeshComponent* B_R01_Axel = nullptr;
	UStaticMeshComponent* B_L03 = nullptr;
	UStaticMeshComponent* B_R03 = nullptr;
	UStaticMeshComponent* B_L02 = nullptr;
	UStaticMeshComponent* B_R02 = nullptr;
	UStaticMeshComponent* B_L01 = nullptr;
	UStaticMeshComponent* B_R01 = nullptr;

	UPROPERTY(EditAnywhere, Category = "Setup")
	UStaticMeshComponent* Body = nullptr;
private:
	TArray<UStaticMeshComponent*> StaticMeshes;

	UStaticMeshComponent* AddStaticMeshComponent(USceneComponent* Parent, FName Name);
	UPhysicsConstraintComponent* AddPhysicsConstraintComponent(USceneComponent* Parent, FName Name);

	void ApplyMeshes();
	void ApplyMaterials();
	void SetLocations();
	void SetUpPhysicsConstraints(UPhysicsConstraintComponent* Component, UStaticMeshComponent* ConstraintActor1, UStaticMeshComponent* ConstraintActor2, bool DisableCollision, ELinearConstraintMotion XMotion, ELinearConstraintMotion YMotion, ELinearConstraintMotion ZMotion, float XYZLimit, EAngularConstraintMotion Swing1Motion, float Swing1MotionLimit, EAngularConstraintMotion Swing2Motion, float Swing2MotionLimit, EAngularConstraintMotion TwistMotion, float TwistMotionLimit, bool VelDriveX, bool VelDriveY, bool VelDriveZ, float Strength);
	void EnablePhysicsOnAll();
};
