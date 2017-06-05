// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "Carriage.h"


// Sets default values
ACarriage::ACarriage()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ACarriage::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACarriage::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

UStaticMeshComponent * ACarriage::GetBackCoupler()
{
	return Back_Coupler;
}

UStaticMeshComponent * ACarriage::GetFrontCoupler()
{
	return Front_Coupler;
}

UStaticMeshComponent * ACarriage::AddStaticMeshComponent(USceneComponent * Parent, FName Name)
{
	UStaticMeshComponent* Ret = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	FAttachmentTransformRules Rules = FAttachmentTransformRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
	Ret->AttachToComponent(Parent, Rules);
	//Ret->RegisterComponent();
	return Ret;
}

UPhysicsConstraintComponent * ACarriage::AddPhysicsConstraintComponent(USceneComponent * Parent, FName Name)
{
	UPhysicsConstraintComponent* Ret = CreateDefaultSubobject<UPhysicsConstraintComponent>(Name);
	FAttachmentTransformRules Rules = FAttachmentTransformRules(EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
	Ret->AttachToComponent(Parent, Rules);
	//Ret->RegisterComponent();
	return Ret;
}

void ACarriage::ApplyMeshes()
{
	ConstructorHelpers::FObjectFinder<UStaticMesh> Carriage_Loader(TEXT("StaticMesh'/Game/Meshes/LandShip_Carriage.LandShip_Carriage'"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> Wheel_Loader(TEXT("StaticMesh'/Game/Meshes/Landship_Wheel.Landship_Wheel'"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cone_Loader(TEXT("StaticMesh'/Game/Meshes/Shape_Cone.Shape_Cone'"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> Cube_Loader(TEXT("StaticMesh'/Game/Meshes/Shape_Cube.Shape_Cube'"));

	if (Carriage_Loader.Succeeded() && Wheel_Loader.Succeeded() && Cone_Loader.Succeeded() && Cube_Loader.Succeeded())
	{
		Body->SetStaticMesh(Carriage_Loader.Object);
		F_L03->SetStaticMesh(Wheel_Loader.Object);
		F_L03_Axel->SetStaticMesh(Cone_Loader.Object);
		F_R03->SetStaticMesh(Wheel_Loader.Object);
		F_R03_Axel->SetStaticMesh(Cone_Loader.Object);
		F_L02_Axel->SetStaticMesh(Cone_Loader.Object);
		F_L02->SetStaticMesh(Wheel_Loader.Object);
		F_R02_Axel->SetStaticMesh(Cone_Loader.Object);
		F_R01->SetStaticMesh(Wheel_Loader.Object);
		F_L01_Axel->SetStaticMesh(Cone_Loader.Object);
		F_R01_Axel->SetStaticMesh(Cone_Loader.Object);
		F_L01->SetStaticMesh(Wheel_Loader.Object);
		F_R02->SetStaticMesh(Wheel_Loader.Object);
		Front_Coupler->SetStaticMesh(Cube_Loader.Object);
		Back_Coupler->SetStaticMesh(Cube_Loader.Object);
		B_L03_Axel->SetStaticMesh(Cone_Loader.Object);
		B_R03_Axel->SetStaticMesh(Cone_Loader.Object);
		B_L02_Axel->SetStaticMesh(Cone_Loader.Object);
		B_R02_Axel->SetStaticMesh(Cone_Loader.Object);
		B_L01_Axel->SetStaticMesh(Cone_Loader.Object);
		B_R01_Axel->SetStaticMesh(Cone_Loader.Object);
		B_L03->SetStaticMesh(Wheel_Loader.Object);
		B_R03->SetStaticMesh(Wheel_Loader.Object);
		B_L02->SetStaticMesh(Wheel_Loader.Object);
		B_R02->SetStaticMesh(Wheel_Loader.Object);
		B_L01->SetStaticMesh(Wheel_Loader.Object);
		B_R01->SetStaticMesh(Wheel_Loader.Object);
	}
}

void ACarriage::ApplyMaterials()
{
	ConstructorHelpers::FObjectFinder<UMaterial> BodyMaterialLoader(TEXT("Material'/Game/Materials/M_Camo.M_Camo'"));
	ConstructorHelpers::FObjectFinder<UMaterial> AxelMaterialLoader(TEXT("Material'/Game/Materials/M_Basic_Wall.M_Basic_Wall'"));

	if (BodyMaterialLoader.Succeeded() && AxelMaterialLoader.Succeeded())
	{
		Body->SetMaterial(0, BodyMaterialLoader.Object);
		F_L03_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		F_R03_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		F_L02_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		F_R02_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		F_L01_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		F_R01_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		Front_Coupler->SetMaterial(0, AxelMaterialLoader.Object);
		Back_Coupler->SetMaterial(0, AxelMaterialLoader.Object);
		B_L03_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		B_R03_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		B_L02_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		B_R02_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		B_L01_Axel->SetMaterial(0, AxelMaterialLoader.Object);
		B_R01_Axel->SetMaterial(0, AxelMaterialLoader.Object);
	}
}

void ACarriage::SetLocations()
{
	B_R01->SetRelativeLocation(FVector(-140.f, 280.f, 0.f));
	B_L01->SetRelativeLocation(FVector(-140.f, -280.f, 0.f));
	B_R02->SetRelativeLocation(FVector(-370.f, 280.f, 0.f));
	B_L02->SetRelativeLocation(FVector(-370.f, -280.f, 0.f));
	B_R03->SetRelativeLocation(FVector(-600.f, 280.f, 0.f));
	B_L03->SetRelativeLocation(FVector(-600.f, -280.f, 0.f));

	B_R01_Axel->SetRelativeLocation(FVector(-140.f, 260.f, 0.f));
	PC_B_R01_Axel->SetRelativeLocation(FVector(-140.f, 260.f, 0.f));
	PC_B_R01->SetRelativeLocation(FVector(-140.f, 280.f, 0.f));

	B_L01_Axel->SetRelativeLocation(FVector(-140.f, -260.f, 0.f));
	PC_B_L01_Axel->SetRelativeLocation(FVector(-140.f, -260.f, 0.f));
	PC_B_L01->SetRelativeLocation(FVector(-140.f, -280.f, 0.f));

	B_R02_Axel->SetRelativeLocation(FVector(-370.f, 260.f, 0.f));
	PC_B_R02_Axel->SetRelativeLocation(FVector(-370.f, 260.f, 0.f));
	PC_B_R02->SetRelativeLocation(FVector(-370.f, 280.f, 0.f));

	B_L02_Axel->SetRelativeLocation(FVector(-370.f, -260.f, 0.f));
	PC_B_L02_Axel->SetRelativeLocation(FVector(-370.f, -260.f, 0.f));
	PC_B_L02->SetRelativeLocation(FVector(-370.f, -280.f, 0.f));

	B_R03_Axel->SetRelativeLocation(FVector(-600.f, 260.f, 0.f));
	PC_B_R03_Axel->SetRelativeLocation(FVector(-600.f, 260.f, 0.f));
	PC_B_R03->SetRelativeLocation(FVector(-600, 280.f, 0.f));

	B_L03_Axel->SetRelativeLocation(FVector(-600.f, -260.f, 0.f));
	PC_L03_Axel->SetRelativeLocation(FVector(-600.f, -260.f, 0.f));
	PC_B_L03->SetRelativeLocation(FVector(-600, -280.f, 0.f));

	Back_Coupler->SetRelativeLocation(FVector(-800.f, 0.f, 50.f));
	Front_Coupler->SetRelativeLocation(FVector(800.f, 0.f, 50.f));
	PC_Front_Coupler->SetRelativeLocation(FVector(700.f, 0.f, 50.f));


	F_R02->SetRelativeLocation(FVector(370.f, 280.f, 0.f));
	PC_F_R01->SetRelativeLocation(FVector(600.f, 280.f, 0.f));

	F_L01->SetRelativeLocation(FVector(600.f, -280.f, 0.f));
	PC_L_R01->SetRelativeLocation(FVector(600.f, -280.f, 0.f));

	F_R01_Axel->SetRelativeLocation(FVector(600.f, 260.f, 0.0));
	PC_F_R01_Axel->SetRelativeLocation(FVector(600.f, 260.f, 0.f));

	F_L01_Axel->SetRelativeLocation(FVector(600.f, -260.f, 0.f));
	PC_F_L01_Axel->SetRelativeLocation(FVector(600.f, -260.f, 0.f));

	F_R01->SetRelativeLocation(FVector(600.f, 280.f, 0.f));
	PC_F_R02->SetRelativeLocation(FVector(370.f, 280.f, 0.f));

	F_R02_Axel->SetRelativeLocation(FVector(370.f, 260.f, 0.f));
	
	F_L02->SetRelativeLocation(FVector(370.f, -280.f, 0.f));
	PC_F_R02_Axel->SetRelativeLocation(FVector(370.f, 260.f, 0.f));
	PC_F_L02->SetRelativeLocation(FVector(370.f, -280.f, 0.f));

	F_L02_Axel->SetRelativeLocation(FVector(370.f, -260.f, 0.f));
	PC_F_L02_Axel->SetRelativeLocation(FVector(370.f, -260.f, 0.f));
	PC_F_R03_Axel->SetRelativeLocation(FVector(140.f, 260.f, 0.f));

	F_R03_Axel->SetRelativeLocation(FVector(140.f, 260.f, 0.f));
	PC_F_R03->SetRelativeLocation(FVector(140.f, 280.f, 0.f));
	F_R03->SetRelativeLocation(FVector(140.f, 280.f, 0.f));

	PC_F_L03_Axel->SetRelativeLocation(FVector(140.f, -260.f, 0.f));
	F_L03_Axel->SetRelativeLocation(FVector(140.f, -260.f, 0.f));
	PC_F_L03->SetRelativeLocation(FVector(140.f, -280.f, 0.f));
	F_L03->SetRelativeLocation(FVector(140.f, -280.f, 0.f));

	PC_Back_Coupler->SetRelativeLocation(FVector(-700.f, 0.f, 50.f));
}

void ACarriage::SetUpPhysicsConstraints(UPhysicsConstraintComponent* Component, UStaticMeshComponent* ConstraintActor1, UStaticMeshComponent* ConstraintActor2, bool DisableCollision, ELinearConstraintMotion XMotion, ELinearConstraintMotion YMotion, ELinearConstraintMotion ZMotion, float XYZLimit, EAngularConstraintMotion Swing1Motion, float Swing1MotionLimit, EAngularConstraintMotion Swing2Motion, float Swing2MotionLimit, EAngularConstraintMotion TwistMotion, float TwistMotionLimit, bool VelDriveX, bool VelDriveY, bool VelDriveZ, float Strength)
{
	Component->SetConstrainedComponents(ConstraintActor1, NAME_None, ConstraintActor2, NAME_None);
	Component->SetDisableCollision(DisableCollision);
	Component->SetLinearXLimit(XMotion, XYZLimit);
	Component->SetLinearYLimit(YMotion, XYZLimit);
	Component->SetLinearZLimit(ZMotion, XYZLimit);

	Component->SetAngularSwing1Limit(Swing1Motion, Swing1MotionLimit);
	Component->SetAngularSwing2Limit(Swing2Motion, Swing2MotionLimit);
	Component->SetAngularTwistLimit(TwistMotion, TwistMotionLimit);

	Component->SetLinearVelocityDrive(VelDriveX, VelDriveY, VelDriveZ);
	Component->SetLinearDriveParams(Strength, 0.f, 0.f);
}

void ACarriage::EnablePhysicsOnAll()
{
	Body->SetCollisionProfileName(FName("VehicleCarriage"));
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetSimulatePhysics(true);
	Body->SetEnableGravity(true);
	for (UStaticMeshComponent* MeshComponent : StaticMeshes)
	{
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetCollisionProfileName("Carriage_Tracks");
		MeshComponent->SetSimulatePhysics(true);
	}
}

