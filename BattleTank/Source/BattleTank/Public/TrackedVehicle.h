// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "Runtime/Engine/Classes/Components/SplineComponent.h"
#include "TrackedVehicle.generated.h"

class ACarriage;

USTRUCT(BlueprintType)
struct FSuspensionInternalProcessing
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	FVector RootLoc = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	FRotator RootRot = FRotator(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	float Length = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	float Radius = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	float Stiffness = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	float Damping = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	float PreviousLength = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	FVector SuspensionForce = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	FVector WheelCollisionLocation = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	FVector WheelCollisionNormal = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	bool Engaged = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuspensionProcessing")
	TEnumAsByte<EPhysicalSurface> HitMaterial = EPhysicalSurface::SurfaceType_Default;
};

USTRUCT(BlueprintType)
struct FSuspensionSetup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSuspensionSetup")
	FVector RootLoc = FVector(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSuspensionSetup")
	FRotator RootRot = FRotator(0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSuspensionSetup")
	float MaximumLength = 23.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSuspensionSetup")
	float CollisionRadius = 34.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSuspensionSetup")
	float StiffnessForce = 4000000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSuspensionSetup")
	float DampingForce = 4000.f;
};

UENUM(BlueprintType)
enum AutoForwardInput
{
	Fwd UMETA(DisplayName="Forward"),
	Neutral UMETA(DisplayName = "Neutral"),
	Backward UMETA(DisplayName = "Backward")
};

UENUM(BlueprintType)
enum Side
{
	Right UMETA(DisplayName = "Right"),
	Left UMETA(DisplayName = "Left")
};

UCLASS()
class BATTLETANK_API ATrackedVehicle : public APawn
{
	GENERATED_BODY()
private:
	


	void AddWheelForce(UPrimitiveComponent* Wheel, FVector Force);
	void AddCarriage(int CarriageType, int Position);
	void AddWheelForceImproved(UPrimitiveComponent* Wheel, FVector Force, FHitResult Hit, UPhysicsConstraintComponent* Suspension);
	void CheckWheelCollision(int SuspIndex, float DeltaTime, TArray<FSuspensionInternalProcessing>& SuspensionArray, Side side);
	FVector GetVelocityAtPoint(FVector PointLoc);
	bool PutToSleep();
	FVector GetVelocityAtPointWorld(FVector PointLoc);
	void TotalSuspensionForce();
	void AddGravity();
	void PositionAndAnimateDriveWheels(UStaticMeshComponent* WheelComponent, FSuspensionInternalProcessing SuspensionSet, int SuspensionIndex, Side side, bool FlipAnimation180Degrees);
	void UpdateThrottle();
	void UpdateWheelsVelocity(float DeltaTime);
	float GetWheelAccelerationFromEngineTorque(float Torque);
	void ApplyDrag();
	float GetEngineTorque(float RPM);
	virtual bool AnimateWheels();
	void UpdateAxleVelocity();
	void CalculateEngineAndUpdateDrive();
	void CountFrictionContactPoint(TArray<FSuspensionInternalProcessing> SuspSide);
	void ApplyDriveForceAndGetFrictionForceOnSide(float& TotalFrictionTorqueSide, float& TotalRollingFrictionTorqueSide, TArray<FSuspensionInternalProcessing> SuspensionSide, FVector DriveForceSide, float TrackLinearVelSide);
	float GetVehicleMass();
	float GetGearBoxTorque(float EngineTorque);
	float GetEngineRPMFromAxle(float AxleAngVel);
	float ApplyBrake(float AngVel_In, float BrakeRatio, float DeltaTime);
	virtual bool AnimateTreadsSpline();
	void AnimateSprocketOrIdler(UStaticMeshComponent* SprocketOrIdlerComponent, float TrackAngVel, bool FlipAnimation180Degrees);
	void ShowSuspensionHandles(bool DebugMode);
	void SpawnDust(TArray < FSuspensionInternalProcessing> SuspensionSide, float TrackLinearVelSlide);
	
	void TraceForSuspension(bool& BlockingHit, FVector& Location, FVector& ImpactPoint, FVector& ImpactNormal, EPhysicalSurface& SurfaceType, UPrimitiveComponent* Component, FVector Start, FVector End, float Radius);
	FORCEINLINE bool VTraceSphere(AActor * ActorToIgnore, const FVector & Start, const FVector & End, const float Radius, FHitResult & HitOut, ECollisionChannel TraceChannel);
	
	virtual bool AnimateTreadsMaterial();
	bool AnimateTreadsInstancedMesh(USplineComponent* SplineR, USplineComponent* SplineL, UInstancedStaticMeshComponent* TreadsR, UInstancedStaticMeshComponent* TreadsL);
	bool AnimateTreadsSplineControlPoints(UStaticMeshComponent* WheelMeshComponent, USplineComponent* TreadSplineComponent, int BottomCPIndex, int TopCPIndex, TArray<FVector>& SplineCoordinates, TArray<FSuspensionSetup> SuspensionSet, int SuspensionIndex);
	void ShiftGear(int ShiftUpOrDown);
	void UpdateAutoGearBox();
	void SetRemoveAutoGearBox(bool ShouldSetTimer);
	void GetThrottleInputForAutoHandling(float InputVehicleLeftRight, float InputVehicleForwardBackward);
	void GetGearBoxInfo(int& GearNum, bool& ReverseGear, bool& Automatic);
	void GetMuFromFrictionElipse(float& Mu_Static, float& Mu_Kinetic, FVector VelocityDirectionNormalized, FVector ForwardVector, float Mu_X_Static, float Mu_Y_Static, float Mu_X_Kinetic, float Mu_Y_Kinetic);
	
public:
	// Sets default values for this pawn's properties
	ATrackedVehicle();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UFUNCTION()
	void ForwardBackward(float AxisValue);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instanced Components")
	UStaticMeshComponent* Body = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instanced Components")
	UArrowComponent* COM = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instanced Components")
	UInstancedStaticMeshComponent* TreadR = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instanced Components")
	UInstancedStaticMeshComponent* TreadL = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instanced Components")
	UStaticMeshComponent* WheelSweep = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float TrackMassKg = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float SprocketMassKg = 65.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float AirDensity = 1.2922f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float DragSurfaceArea = 10.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float DragCoef = 0.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float SprocketRadiusCm = 24.05f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	TArray<float> GearRatio;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float DifferentialRatio = 3.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	UCurveFloat* EngineTorqueCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float TransmissionEfficiency = 0.9f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float Mu_X_Static = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float Mu_Y_Static = 0.85f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float Mu_X_Kinetic = 0.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float Mu_Y_Kinetic = 0.45f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float RollingFrictionCoef = 0.02f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float BrakeForce = 30.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float TreadLength = 972.469971f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	TArray<FVector> SplineCoordinatesR;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	TArray<FVector> SplineCoordinatesL;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	TArray<FVector> SplineTangents;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float TreadUvTiles = 32.5;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float TreadsOnSide = 64.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float TreadHalfThickness = 2.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	TArray<FSuspensionSetup> FSuspensionSetupR;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	TArray<FSuspensionSetup> FSuspensionSetupL;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float SleepVelocity = 5.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float SleepTimerSeconds = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	bool AutoGearBox = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float GearUpShiftPrc = 0.9f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float GearDownShiftPrc = 0.05f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	float EngineExtraPowerRatio = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	USplineComponent* TrackSplineR = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Public")
	USplineComponent* TrackSplineL = nullptr;



	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackTorqueTransferRight = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackTorqueTransferLeft = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float WheelRightCoefficient = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float WheelLeftCoefficient = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	UStaticMeshComponent* BackCarriageCoupler = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	TArray<ACarriage*> Carriages;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	TArray<FSuspensionInternalProcessing> SuspensionsInternalRight;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	TArray<FSuspensionInternalProcessing> SuspensionsInternalLeft;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float SuspTargetVelocity = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float WheelForwardCoefficient = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackRightAngVel = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackLeftAngVel = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackRightLinVel = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackLeftLinVel = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	int CurrentGear = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float Throttle = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float ThrottleIncrement = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackRightTorque = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackLeftTorque = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float DriveRightTorque = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float DriveLeftTorque = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float MomentIntertia = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float DriveAxleTorque = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float AxleAngVel = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float BrakeRatioRight = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float BrakeRatioLeft = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	FVector DriveRightForce;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	FVector DriveLeftForce;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TotNumFrictionPoints = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TotalSupsForceLeft = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	bool ReverseGear = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackFrictionTorqueRight = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackFrictionTorqueLeft = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float EngineRPM = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float EngineTorque = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackRollingFrictionTorqueRight = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TrackRollingFrictionTorqueLeft = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	UMaterialInstanceDynamic* TreadMaterialRight;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	UMaterialInstanceDynamic* TreadMaterialLeft;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TreadUVOffsetRight = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TreadUVOffsetLeft = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TreadMeshOffsetRight = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float TreadMeshOffsetLeft = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	int TreadsLastIndex = 63;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float SplineLengthAtConstruction = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	bool SleepMod = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float SleepDelayTimer = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	FTimerHandle AutoGearBoxTimerHandle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	float LastAutoGearBoxAxleCheck = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	TEnumAsByte<AutoForwardInput> AutoForwardHandle = AutoForwardInput::Fwd;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Internals")
	int NeutralGearIndex = 0;





	// Instanced Stuff...
	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> SuspHandleRight;
	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> SuspHandleLeft;


	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> SprocketsRight;
	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> SprocketsLeft;
	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> IdlersRight;
	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> IdlersLeft;
	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> RightWheels;
	UPROPERTY(BlueprintReadWrite, Category = "Setup")
	TArray<UStaticMeshComponent*> LeftWheels;

	bool DebugMode = false;
};
