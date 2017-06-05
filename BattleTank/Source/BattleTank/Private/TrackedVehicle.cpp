// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "TrackedVehicle.h"
#include "Carriage.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TankAimingComponent.h"
#include "TankMovementComponent.h"
#include "TankBarrel.h"
#include "TankTurret.h"
#include "Projectile.h"


// Sets default values
ATrackedVehicle::ATrackedVehicle()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TankAimingComponent = CreateDefaultSubobject<UTankAimingComponent>(FName("Aiming Component"));
}

// Called when the game starts or when spawned
void ATrackedVehicle::BeginPlay()
{
	Super::BeginPlay();
	
	SetRemoveAutoGearBox(true);

	for (int i = 0; i < GearRatio.Num(); i++)
	{
		if (GearRatio[i] == 0)
		{
			NeutralGearIndex = i;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Neutral Gear Not Found!"));
		}
	}

	if (AutoGearBox)
	{
		CurrentGear = NeutralGearIndex + 1;
	}
	else
	{
		CurrentGear = NeutralGearIndex;
	}
}

// Called every frame
void ATrackedVehicle::Tick( float DeltaTime ) // TODO DoubleCheck tick function is correct...
{
	Super::Tick( DeltaTime );

	if (!Body)
		return;
	if (!TrackSplineR)
		return;
	if(!TrackSplineL)
		return;
	if(!TreadR)
		return;
	if(!TreadL)
		return;
	if (RightWheels.Num() == 0)
		return;
	if (LeftWheels.Num() == 0)
		return;

	if (!PutToSleep())
	{
		UpdateThrottle(DeltaTime);
		UpdateWheelsVelocity(DeltaTime);
		AnimateWheels();
		AnimateTreadsMaterial(DeltaTime);
		AnimateTreadsSpline();
		AnimateTreadsInstancedMesh(TrackSplineR, TrackSplineL, TreadR, TreadL);
		UpdateAxleVelocity();
		CalculateEngineAndUpdateDrive();

		for (int i = 0; i < SuspensionsInternalRight.Num(); i++)
		{
			CheckWheelCollision(i, DeltaTime, SuspensionsInternalRight, Side::Right);
		}
		
		for (int i = 0; i < SuspensionsInternalLeft.Num(); i++)
		{
			CheckWheelCollision(i, DeltaTime, SuspensionsInternalLeft, Side::Left);
		}

		CountFrictionContactPoint(SuspensionsInternalRight);
		CountFrictionContactPoint(SuspensionsInternalLeft);

		ApplyDriveForceAndGetFrictionForceOnSide(TrackFrictionTorqueRight, TrackRollingFrictionTorqueRight, SuspensionsInternalRight, TrackRightLinVel, DriveRightForce, DeltaTime);
		ApplyDriveForceAndGetFrictionForceOnSide(TrackFrictionTorqueLeft, TrackRollingFrictionTorqueLeft, SuspensionsInternalLeft, TrackLeftLinVel, DriveLeftForce, DeltaTime);

		//SpawnDust(SuspensionsInternalRight, this->TrackRightLinVel);
		//SpawnDust(SuspensionsInternalLeft, this->TrackLeftLinVel);

		this->TotNumFrictionPoints = 0.f;
	}
	
	if (DebugMode && SleepMod)
	{
		DrawDebugString(GetWorld(), GetActorLocation(), "Sleeping");
	}
}

// Called to bind functionality to input
void ATrackedVehicle::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	//InputComponent->BindAxis(FName("FWD"), this, &ATrackedVehicle::ForwardBackward);
	//InputComponent->BindAxis(FName("Rotate"), this, &ATrackedVehicle::LeftRight);
}

FVector ATrackedVehicle::GetCenterOfMass()
{
	return Body->GetCenterOfMass();
}

void ATrackedVehicle::ForwardBackward(float AxisValue)
{
	if (AutoGearBox)
	{
		GetThrottleInputForAutoHandling(AxisValueY, AxisValue);
	}
	else
	{
		WheelForwardCoefficient = FMath::Clamp<float>(AxisValue, 0.f, 1.f);

		if (AxisValue < 0.f)
		{
			BrakeRatioRight = AxisValue*(-1);
			BrakeRatioLeft = AxisValue*(-1);
		}
		else
		{
			BrakeRatioRight = AxisValue;
			BrakeRatioLeft = AxisValue;
		}
	}

	if (AxisValue != 0.f)
	{
		SleepMod = false;
		SleepDelayTimer = 0.f;
	}
}

void ATrackedVehicle::LeftRight(float AxisValue)
{
	//float AxisValueX = GetWorld()->GetFirstPlayerController()->InputComponent->GetAxisValue(FName("FWD"));
	AxisValueY = AxisValue;
	if (AxisValue < 0)
	{
		WheelRightCoefficient = FMath::Abs(AxisValue);
		WheelLeftCoefficient = FMath::Abs(AxisValue)*(-1);
		BrakeRatioLeft = 0.f;
		BrakeRatioRight = 0.f;
	}
	else
	{
		WheelRightCoefficient = FMath::Abs(AxisValue)*(-1);
		WheelLeftCoefficient = FMath::Abs(AxisValue);
		BrakeRatioLeft = 0.f;
		BrakeRatioRight = 0.f;
	}

	if (AxisValue != 0.f)
	{
		SleepMod = false;
		SleepDelayTimer = 0.f;
	}
}

void ATrackedVehicle::SetBarrelReference(UTankBarrel * BarrelToSet)
{
	if (TankAimingComponent)
		TankAimingComponent->SetBarrelReference(BarrelToSet);
	Barrel = BarrelToSet;
}

void ATrackedVehicle::SetTurretReference(UTankTurret * TurretToSet)
{
	if (TankAimingComponent)
		TankAimingComponent->SetTurretReference(TurretToSet);
}

void ATrackedVehicle::Fire()
{
	bool isReloaded = (FPlatformTime::Seconds() - LastFireTime) > ReloadTimeInSeconds;
	if (Barrel && isReloaded)
	{

		// Spawn a projectile at the socket location on the barrel
		FActorSpawnParameters p;
		AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileBlueprint, Barrel->GetSocketLocation(FName("Projectile")), Barrel->GetSocketRotation(FName("Projectile")));

		Projectile->LaunchProjectile(LaunchSpeed);

		LastFireTime = FPlatformTime::Seconds();
	}
}

void ATrackedVehicle::AimAt(FVector HitLocation)
{
	if (TankAimingComponent)
		TankAimingComponent->AimAt(HitLocation, LaunchSpeed);
}

bool ATrackedVehicle::VTraceSphere(AActor * ActorToIgnore, const FVector & Start, const FVector & End, const float Radius, FHitResult & HitOut, ECollisionChannel TraceChannel)
{
	FCollisionQueryParams TraceParams(FName(TEXT("VictoreCore Trace")), true, ActorToIgnore);
	TraceParams.bTraceComplex = false;
	//TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	//Ignore Actors
	TraceParams.AddIgnoredActor(ActorToIgnore);

	//Re-initialize hit info
	HitOut = FHitResult(ForceInit);

	//Get World Source
	TObjectIterator< APlayerController > ThePC;
	if (!ThePC) return false;


	return ThePC->GetWorld()->SweepSingleByChannel(
		HitOut,
		Start,
		End,
		FQuat(),
		TraceChannel,
		FCollisionShape::MakeSphere(Radius),
		TraceParams
	);
}

void ATrackedVehicle::AddWheelForce(UPrimitiveComponent * Wheel, FVector Force)
{
	FTransform RelativeTransform = FTransform(Wheel->RelativeRotation, Wheel->RelativeLocation, Wheel->RelativeScale3D);

	FHitResult Hit;

	FCollisionQueryParams QueryParams = FCollisionQueryParams::DefaultQueryParam;
	QueryParams.AddIgnoredActor(this);

	FVector TransformedVector = FVector(0, 0, -1);
	TransformedVector = RelativeTransform.TransformVector(TransformedVector);
	GetWorld()->LineTraceSingleByChannel(Hit, Wheel->GetComponentLocation(), Wheel->GetComponentLocation() + (TransformedVector * 110), ECollisionChannel::ECC_Visibility, QueryParams);

	if (Hit.bBlockingHit)
	{
		Force = RelativeTransform.TransformVector(Force);
		Wheel->AddForce(Force);
	}
}

void ATrackedVehicle::AddCarriage(int CarriageType, int Position)
{
	if (Position == 1)
	{
		FVector SpawnPosition = FVector(-1700, 0, 0)*Position;
		GetActorRotation().RotateVector(SpawnPosition);
		SpawnPosition += GetActorLocation();

		int CarIndex = Carriages.Add(GetWorld()->SpawnActor<ACarriage>(SpawnPosition, GetActorRotation()));

		SpawnPosition = FVector(-1650, 0, 0)*Position;
		GetActorRotation().RotateVector(SpawnPosition);
		SpawnPosition += GetActorLocation();

		APhysicsConstraintActor* Constraint = GetWorld()->SpawnActor<APhysicsConstraintActor>(SpawnPosition, GetActorRotation());

		UStaticMeshComponent* BackCoupler = Carriages[CarIndex]->GetBackCoupler();
		
		FAttachmentTransformRules Rules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);

		Constraint->AttachToComponent(BackCoupler, Rules);

		UStaticMeshComponent* FrontCoupler = Carriages[CarIndex]->GetFrontCoupler();
		Constraint->ConstraintComp->SetConstrainedComponents(BackCoupler, NAME_None, FrontCoupler, NAME_None);
	}
}

void ATrackedVehicle::AddWheelForceImproved(UPrimitiveComponent* Wheel, FVector Force, FHitResult Hit, UPhysicsConstraintComponent* Suspension)
{
	FTransform RelativeTransform = FTransform(GetActorRotation(), Wheel->RelativeLocation, Wheel->RelativeScale3D);
	
	Force = RelativeTransform.TransformVector(Force);

	Body->AddForceAtLocation(Force, Hit.Location);

	FVector OutLinearForce, OutAngularForce;
}

void ATrackedVehicle::CheckWheelCollision(int SuspIndex, float DeltaTime, TArray<FSuspensionInternalProcessing>& SuspensionArray, Side side)
{
	if (!Body)
		return;

	float WheelAngVelocity = TrackLeftAngVel;
	float WheelLinVelocity = TrackLeftLinVel;

	if (side == Side::Right)
	{
		WheelAngVelocity = TrackRightAngVel;
		WheelLinVelocity = TrackRightLinVel;
	}

	float SuspLength = SuspensionArray[SuspIndex].Length;
	float SuspPreviousLength = SuspensionArray[SuspIndex].PreviousLength;
	float SuspStiffness = SuspensionArray[SuspIndex].Stiffness;
	float SuspDamping = SuspensionArray[SuspIndex].Damping;
	
	FTransform ActorTransform = GetActorTransform();

	FVector UpVector = SuspensionArray[SuspIndex].RootRot.RotateVector(FVector::UpVector);
	FVector RightVector = SuspensionArray[SuspIndex].RootRot.RotateVector(FVector::RightVector);
	FVector ForwardVector = SuspensionArray[SuspIndex].RootRot.RotateVector(FVector::ForwardVector);

	FVector SuspWorldZVector = ActorTransform.TransformVector(UpVector);
	FVector SuspWorldYVector = ActorTransform.TransformVector(RightVector);
	FVector SuspWorldXVector = ActorTransform.TransformVector(ForwardVector);

	FVector SuspWorldLocation = ActorTransform.TransformPosition(SuspensionArray[SuspIndex].RootLoc);

	FHitResult Hit;

	// TODO check if this is the reason for wonky physics...
	TraceForSuspension(Hit, SuspWorldLocation, SuspWorldLocation + SuspWorldZVector*(-1.f)*SuspLength, SuspensionArray[SuspIndex].Radius);

	bool BlockingHit = Hit.bBlockingHit;
	FVector Location = Hit.Location;
	FVector ImpactPoint = Hit.ImpactPoint;
	FVector ImpactNormal = Hit.ImpactNormal;
	EPhysicalSurface SurfaceType;
	if (Hit.PhysMaterial.Get())
	{
		SurfaceType = Hit.PhysMaterial.Get()->SurfaceType;
	}

	UPrimitiveComponent* CollisionPrimitive = nullptr;

	if (Hit.GetComponent())
	{
		CollisionPrimitive = Hit.GetComponent();
	}

	if (BlockingHit)
	{
		bool SuspEngaged = true;
		float SuspNewLength = (SuspWorldLocation - Location).Size();

		FVector WheelCollisionLocation = ImpactPoint;
		FVector WheelCollisionNormal = ImpactNormal;

		// 0
		float SpringCompressionRatio = FMath::Clamp<float>((SuspLength - SuspNewLength) / SuspLength, 0.f, 1.f)*SuspStiffness;
		float Vel1 = (SuspNewLength - SuspPreviousLength) / DeltaTime;

		float SuspensionVelocity = (SuspTargetVelocity - Vel1)*SuspDamping;
		FVector SuspensionForce = (SpringCompressionRatio + SuspensionVelocity)*SuspWorldZVector;


		// TODO Debug Suspension force
		Body->AddForceAtLocation(SuspensionForce, SuspWorldLocation);

		FSuspensionInternalProcessing Susp = SuspensionArray[SuspIndex];
		Susp.PreviousLength = SuspNewLength;
		Susp.SuspensionForce = SuspensionForce;
		Susp.WheelCollisionLocation = WheelCollisionLocation;
		Susp.WheelCollisionNormal = WheelCollisionNormal;
		Susp.Engaged = SuspEngaged;
		Susp.HitMaterial = SurfaceType;

		SuspensionArray[SuspIndex] = Susp;

		if (CollisionPrimitive)
		{
			if (CollisionPrimitive->IsSimulatingPhysics())
			{
				CollisionPrimitive->AddForceAtLocation(SuspensionForce*(-1), WheelCollisionLocation);
			}
		}

		DrawDebugLine(GetWorld(), SuspWorldLocation, SuspWorldLocation + SuspensionForce*0.0001f, FLinearColor(0.f, 1.f, 0.f).ToFColor(true), false, -1.f, 0, 5.f);
		DrawDebugPoint(GetWorld(), WheelCollisionLocation, 5, FLinearColor(0.843f, 0.f, 0.935f).ToFColor(true), false);
		DrawDebugLine(GetWorld(), SuspWorldLocation, SuspWorldLocation + SuspWorldZVector*(-1)*SuspNewLength, (FLinearColor(0.f, 0.f, 1.f).ToFColor(true)), false, -1.f, 0, 5.f);
		DrawDebugLine(GetWorld(), SuspWorldLocation, SuspWorldLocation + SuspWorldZVector*(-1)*SuspLength, FLinearColor(0.f, 0.f, 1.f).ToFColor(true), false, -1.f, 0, 2.5f);
	}
	else
	{
		FVector SuspensionForce = FVector::ZeroVector;
		float SuspNewLength = SuspLength;
		FVector WheelCollisionLocation = FVector::ZeroVector;
		FVector WheelCollisionNormal = FVector::ZeroVector;
		bool SuspEngaged = false;

		FSuspensionInternalProcessing Susp = SuspensionArray[SuspIndex];
		Susp.PreviousLength = SuspNewLength;
		Susp.SuspensionForce = SuspensionForce;
		Susp.WheelCollisionLocation = WheelCollisionLocation;
		Susp.WheelCollisionNormal = WheelCollisionNormal;
		Susp.Engaged = SuspEngaged;
		Susp.HitMaterial = SurfaceType;

		SuspensionArray[SuspIndex] = Susp;

		if (CollisionPrimitive)
		{
			if (CollisionPrimitive->IsSimulatingPhysics())
			{
				CollisionPrimitive->AddForceAtLocation(SuspensionForce*(-1), WheelCollisionLocation);
			}
		}
	}
}

FVector ATrackedVehicle::GetVelocityAtPoint(FVector PointLoc)
{
	FVector LinVel = Body->GetPhysicsLinearVelocity();
	FVector AngVel = Body->GetPhysicsAngularVelocity();
	FVector CenterOfMass = GetCenterOfMass();

	FTransform Transform = FTransform(GetActorRotation(), CenterOfMass, GetActorScale3D());

	FVector InvTransformedAngularVelocity = Transform.InverseTransformVector(AngVel);
	FVector InvTransformedPosition = Transform.InverseTransformVector(PointLoc);

	InvTransformedAngularVelocity.X = FMath::DegreesToRadians(InvTransformedAngularVelocity.X);
	InvTransformedAngularVelocity.X = FMath::DegreesToRadians(InvTransformedAngularVelocity.Y);
	InvTransformedAngularVelocity.X = FMath::DegreesToRadians(InvTransformedAngularVelocity.Z);

	FVector ResultAngVel = FVector::CrossProduct(InvTransformedAngularVelocity, InvTransformedPosition);

	FVector InvTransformedLinearVelocity = GetActorTransform().InverseTransformVector(LinVel);

	return GetActorTransform().TransformVector(InvTransformedLinearVelocity + ResultAngVel);
}

bool ATrackedVehicle::PutToSleep()
{
	if (!Body)
		return SleepMod;

	if (SleepMod)
	{
		if (Body->GetPhysicsLinearVelocity().Size() < SleepVelocity && Body->GetPhysicsAngularVelocity().Size() < SleepVelocity)
		{
			if (!SleepMod)
			{
				SleepMod = true;

				Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()))->PutRigidBodyToSleep();
				SleepDelayTimer = 0.f;

				return SleepMod;
			}
			else
			{
				return SleepMod;
			}
		}
		else
		{
			if (SleepMod)
			{
				SleepMod = false;
				SleepDelayTimer = 0.f;
			}

			return SleepMod;
		}
	}
	else
	{
		if (SleepDelayTimer >= SleepTimerSeconds)
		{
			if (Body->GetPhysicsLinearVelocity().Size() < SleepVelocity && Body->GetPhysicsAngularVelocity().Size() < SleepVelocity)
			{
				if (!SleepMod)
				{
					SleepMod = true;

					Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()))->PutRigidBodyToSleep();
					SleepDelayTimer = 0.f;

					return SleepMod;
				}
				else
				{
					return SleepMod;
				}
			}
			else
			{
				if (SleepMod)
				{
					SleepMod = false;
					SleepDelayTimer = 0.f;
				}

				return SleepMod;
			}
		}
		else
		{
			SleepDelayTimer += GetWorld()->GetDeltaSeconds();
			return SleepMod;
		}
	}

	return SleepMod;
}

FVector ATrackedVehicle::GetVelocityAtPointWorld(FVector PointLoc)
{
	FTransform ActorTransform = GetActorTransform();
	FVector PlaneLocalVelocity = ActorTransform.InverseTransformVector(Body->GetPhysicsLinearVelocity());
	FVector PlaneAngularVelocity = ActorTransform.InverseTransformVector(Body->GetPhysicsAngularVelocity());
	FVector CenterOfMass = ActorTransform.InverseTransformPosition(GetCenterOfMass());
	FVector Location = ActorTransform.InverseTransformPosition(PointLoc);

	FVector ResVel = PlaneLocalVelocity + (PlaneAngularVelocity*0.0174532925f*(Location - CenterOfMass));

	return ActorTransform.TransformVector(ResVel);
}

void ATrackedVehicle::TotalSuspensionForce()
{
	// Debugging
}

void ATrackedVehicle::AddGravity()
{
	Body->AddForce(Body->GetMass()*FVector(0, 0, -981.f));
}

void ATrackedVehicle::PositionAndAnimateDriveWheels(UStaticMeshComponent* WheelComponent, FSuspensionInternalProcessing SuspensionSet, int SuspensionIndex, Side side, bool FlipAnimation180Degrees)
{
	if (!WheelComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't animate wheel!?"));
		return;
	}

	float AngVel;
	FTransform Transform = FTransform(SuspensionSet.RootRot, SuspensionSet.RootLoc, FVector(1.f, 1.f, 1.f));
	
	FVector WorldLocation = GetActorTransform().TransformPosition(Transform.TransformPosition(FVector(0, 0, SuspensionSet.PreviousLength*-1)));

	if (side == Side::Right)
		AngVel = TrackRightAngVel;
	if (side == Side::Left)
		AngVel = TrackLeftAngVel;

	if(FlipAnimation180Degrees)
		AngVel = FMath::RadiansToDegrees(AngVel)*GetWorld()->GetDeltaSeconds();
	else
		AngVel = FMath::RadiansToDegrees(AngVel)*GetWorld()->GetDeltaSeconds()*(-1);

	WheelComponent->SetWorldLocation(WorldLocation);
	WheelComponent->AddLocalRotation(FRotator(AngVel, 0, 0));
}

void ATrackedVehicle::UpdateThrottle(float DeltaTime)
{
	TrackTorqueTransferRight = FMath::Clamp<float>(WheelRightCoefficient + WheelForwardCoefficient, -1.f, 2.f);
	TrackTorqueTransferLeft = FMath::Clamp<float>(WheelLeftCoefficient + WheelForwardCoefficient, -1.f, 2.f);

	float Max = FMath::Max<float>(TrackTorqueTransferLeft, TrackTorqueTransferRight);

	if (Max != 0.f)
	{
		ThrottleIncrement = 0.5f;
	}
	else
	{
		ThrottleIncrement = -1.f;
	}

	Throttle = FMath::Clamp<float>(Throttle + ThrottleIncrement*DeltaTime, 0.f, 1.f);
}

void ATrackedVehicle::UpdateWheelsVelocity(float DeltaTime)
{
	TrackRightTorque = DriveRightTorque + TrackFrictionTorqueRight + TrackRollingFrictionTorqueRight;
	TrackLeftTorque = DriveLeftTorque + TrackFrictionTorqueLeft + TrackRollingFrictionTorqueLeft;

	float AngVelInRight = (TrackRightTorque / MomentIntertia) * DeltaTime + TrackRightAngVel;
	float AngVelInLeft = (TrackLeftTorque / MomentIntertia) * DeltaTime + TrackLeftAngVel;

	TrackRightAngVel = ApplyBrake(AngVelInRight, BrakeRatioRight, DeltaTime);	// TODO, doublecheck
	TrackLeftAngVel = ApplyBrake(AngVelInLeft, BrakeRatioLeft, DeltaTime);		// TODO, doublecheck

	TrackRightLinVel = TrackRightAngVel * SprocketRadiusCm;
	TrackLeftLinVel = TrackLeftAngVel * SprocketRadiusCm;
}

float ATrackedVehicle::GetWheelAccelerationFromEngineTorque(float Torque)
{
	return (Torque / MomentIntertia);
}

void ATrackedVehicle::ApplyDrag()
{
	float Velocity = GetVelocity().Size();
	FVector Drag = (Velocity*0.036*Velocity*0.036*AirDensity*DragSurfaceArea*DragCoef*27.7777778f)*GetVelocity().GetSafeNormal()*-1;

	Body->AddForce(Drag);
}

float ATrackedVehicle::GetEngineTorque(float RPM)
{
	float minTime, maxTime;
	EngineTorqueCurve->GetTimeRange(minTime, maxTime);

	EngineRPM = FMath::Clamp<float>(RPM, minTime, maxTime);

	
	return EngineTorqueCurve->GetFloatValue(EngineRPM) * 100;
}

bool ATrackedVehicle::AnimateWheels() // Virtual
{
	//if (RightWheels.Num() != SuspensionsInternalRight.Num())
	//	return false;
	//if (LeftWheels.Num() != SuspensionsInternalLeft.Num())
	//	return false;

	// Animate Right Wheels
	for (int i = 0; i < SuspensionsInternalRight.Num(); i++)
	{
		PositionAndAnimateDriveWheels(RightWheels[i], SuspensionsInternalRight[i], i, Side::Right, false);
	}
	// Animate Left Wheels
	for (int i = 0; i < SuspensionsInternalLeft.Num(); i++)
	{
		PositionAndAnimateDriveWheels(LeftWheels[i], SuspensionsInternalLeft[i], i, Side::Left, false);
	}
	// Animate Sprockets Right
	for (UStaticMeshComponent* Sprocket : SprocketsRight)
	{
		AnimateSprocketOrIdler(Sprocket, TrackRightAngVel, false);
	}

	// Animate Sprockets Left
	for (UStaticMeshComponent* Sprocket : SprocketsLeft)
	{
		AnimateSprocketOrIdler(Sprocket, TrackLeftAngVel, false);
	}

	// Animate Idlers Right
	for (UStaticMeshComponent* Idler : IdlersRight)
	{
		AnimateSprocketOrIdler(Idler, TrackRightAngVel, false);
	}
	// Animate Idlers Left
	for (UStaticMeshComponent* Idler : IdlersLeft)
	{
		AnimateSprocketOrIdler(Idler, TrackLeftAngVel, false);
	}

	return false;
}

void ATrackedVehicle::UpdateAxleVelocity()
{
	AxleAngVel = (FMath::Abs(TrackRightAngVel) + FMath::Abs(TrackLeftAngVel)) / 2.f;
}

void ATrackedVehicle::CalculateEngineAndUpdateDrive()
{
	float CurrentEngineRPM = GetEngineRPMFromAxle(AxleAngVel);
	EngineTorque = GetEngineTorque(CurrentEngineRPM)*Throttle;
	DriveAxleTorque = GetGearBoxTorque(EngineTorque);

	DriveRightTorque = TrackTorqueTransferRight * DriveAxleTorque;
	DriveLeftTorque = TrackTorqueTransferLeft * DriveAxleTorque;

	DriveRightForce = GetActorForwardVector()*(DriveRightTorque / SprocketRadiusCm);
	DriveLeftForce = GetActorForwardVector()*(DriveLeftTorque / SprocketRadiusCm);
}

void ATrackedVehicle::CountFrictionContactPoint(TArray<FSuspensionInternalProcessing> SuspSide)
{
	for (FSuspensionInternalProcessing Susp : SuspSide)
	{
		if (Susp.Engaged)
		{
			TotNumFrictionPoints++;
		}
	}
}

void ATrackedVehicle::ApplyDriveForceAndGetFrictionForceOnSide(float& TotalFrictionTorqueSide, float& TotalRollingFrictionTorqueSide, TArray<FSuspensionInternalProcessing> SuspensionSide, float TrackLinearVelSide, FVector DriveForceSide, float DeltaTime)
{
	float MaxTransferForceX = 0, TrackTractionTorque = 0, TotalTrackFrictionTorque = 0, TrackFrictionTorque = 0, VehicleMass = 0, WheelLoadN = 0, MaxTransferForceY = 0, TrackRollingFrictionTorque = 0, TotalTrackRollingFrictionTorque = 0, MuStatic = 0, MuKinetic = 0;
	FVector RelativeTrackVel, FullStaticFrictionForce, FullStaticDriveForce, ApplicationForce, RollingFriction, FullDriveForceNorm, FullFrictionForceNorm, FrictionForceX, FrictionForceY, FullKineticFrictionForce, FullKineticDriveForce;

	for (FSuspensionInternalProcessing Suspension : SuspensionSide)
	{
		if(Suspension.Engaged)
		{
			WheelLoadN = Suspension.SuspensionForce.ProjectOnTo(Suspension.WheelCollisionNormal).Size();

			// Get calculate the relative track velocity
			RelativeTrackVel = FVector::VectorPlaneProject(GetVelocityAtPointWorld(Suspension.WheelCollisionLocation) - (GetActorForwardVector()*TrackLinearVelSide), Suspension.WheelCollisionNormal);
			
			GetMuFromFrictionElipse(MuStatic, MuKinetic, RelativeTrackVel.GetSafeNormal(), GetActorForwardVector(), Mu_X_Static, Mu_Y_Static, Mu_X_Kinetic, Mu_Y_Kinetic);

			VehicleMass = GetVehicleMass();

			FVector ProjectedForwardVector = FVector::VectorPlaneProject(GetActorForwardVector(), Suspension.WheelCollisionNormal).GetSafeNormal();
			FVector ProjectedRightVector = FVector::VectorPlaneProject(GetActorRightVector(), Suspension.WheelCollisionNormal).GetSafeNormal();

			FVector RelativeTrackVelPerFrame = ((RelativeTrackVel*(-1)*VehicleMass) / DeltaTime) / TotNumFrictionPoints;

			FVector ProjectedXTrackVelPerFrame = RelativeTrackVelPerFrame.ProjectOnTo(ProjectedForwardVector);
			FVector ProjectedYTrackVelPerFrame = RelativeTrackVelPerFrame.ProjectOnTo(ProjectedRightVector);

			FullStaticFrictionForce = ProjectedXTrackVelPerFrame*Mu_X_Static + ProjectedYTrackVelPerFrame*Mu_Y_Static;
			FullKineticFrictionForce = ProjectedXTrackVelPerFrame*Mu_X_Kinetic + ProjectedYTrackVelPerFrame*Mu_Y_Kinetic;

			// Drive force from transmission torque
			FVector PlaneProjectedDriveForce = FVector::VectorPlaneProject(DriveForceSide, Suspension.WheelCollisionNormal);

			FullStaticDriveForce = PlaneProjectedDriveForce*Mu_X_Static;
			FullKineticDriveForce = PlaneProjectedDriveForce*Mu_X_Kinetic;

			FVector CombinedStaticFrictionForce = FullStaticFrictionForce + FullStaticDriveForce;
			FVector CombinedKineticFrictionForce = FullKineticFrictionForce + FullKineticDriveForce;

			if (CombinedStaticFrictionForce.Size() >= WheelLoadN*MuStatic)
			{
				FullFrictionForceNorm = FullKineticFrictionForce.GetSafeNormal();
				ApplicationForce = CombinedKineticFrictionForce.GetClampedToSize(0.f, WheelLoadN*MuKinetic);
			}
			else
			{
				FullFrictionForceNorm = FullStaticFrictionForce.GetSafeNormal();
				ApplicationForce = CombinedStaticFrictionForce.GetClampedToSize(0.f, WheelLoadN*MuStatic);
			}

			Body->AddForceAtLocation(ApplicationForce, Suspension.WheelCollisionLocation);
			DrawDebugLine(GetWorld(), Suspension.WheelCollisionLocation, Suspension.WheelCollisionLocation + ApplicationForce*0.001f, FLinearColor(0.f, 0.501f, 0.515f).ToFColor(true), false, 0.f, 0, 10.f);

			FVector TransmissionAffector = (ApplicationForce.ProjectOnTo(FullFrictionForceNorm)*(TrackMassKg+SprocketMassKg)*(-1)) / VehicleMass;

			TrackFrictionTorque = GetActorTransform().InverseTransformVector(TransmissionAffector).ProjectOnTo(FVector(1.f, 0.f, 0.f)).X*SprocketRadiusCm;

			TrackRollingFrictionTorque = (WheelLoadN*RollingFrictionCoef*FMath::Sign(TrackLinearVelSide)*(-1)) + (FMath::Sign(TrackLinearVelSide)*(-1)*FMath::Abs(TrackLinearVelSide)*0.000015f*WheelLoadN);

			TotalTrackFrictionTorque += TrackFrictionTorque;
			TotalTrackRollingFrictionTorque += TrackRollingFrictionTorque;
		}
	}

	TotalFrictionTorqueSide = TotalTrackFrictionTorque;
	TotalRollingFrictionTorqueSide = TotalTrackRollingFrictionTorque;
}

float ATrackedVehicle::GetVehicleMass()
{
	return Body->GetMass();
}

float ATrackedVehicle::GetGearBoxTorque(float EngineTorque)
{
	//UE_LOG(LogTemp, Warning, TEXT("CurrentGear: %i"), CurrentGear);
	float Mul;
	if (ReverseGear)
		Mul = -1.f;
	else
		Mul = 1.f;

	return EngineTorque*GearRatio[CurrentGear] * DifferentialRatio*TransmissionEfficiency*Mul*EngineExtraPowerRatio;
}

float ATrackedVehicle::GetEngineRPMFromAxle(float AxleAngVel)
{
	return (GearRatio[CurrentGear] * DifferentialRatio*AxleAngVel*60.f) / (2 * PI);
}

float ATrackedVehicle::ApplyBrake(float AngVel_In, float BrakeRatio, float DeltaTime)
{
	float FrameBreakForce = BrakeRatio*BrakeForce*DeltaTime;
	float NewVel;

	if (FMath::Abs(AngVel_In) > FMath::Abs(FrameBreakForce))
	{
		NewVel = AngVel_In - FrameBreakForce*FMath::Sign(AngVel_In);
	}
	else
	{
		NewVel = 0.f;
	}

	return NewVel;
}

bool ATrackedVehicle::AnimateTreadsSpline() // Inherited later
{
	int BottomCPIndex = 3;
	int TopCPIndex = 14;

	for (int i = 0; i < RightWheels.Num(); i++)
	{
		AnimateTreadsSplineControlPoints(RightWheels[i], TrackSplineR, BottomCPIndex++, TopCPIndex--, SplineCoordinatesR, FSuspensionSetupR, i);
	}
	
	BottomCPIndex = 3;
	TopCPIndex = 14;

	for (int i = 0; i < LeftWheels.Num(); i++)
	{
		AnimateTreadsSplineControlPoints(LeftWheels[i], TrackSplineL, BottomCPIndex++, TopCPIndex--, SplineCoordinatesL, FSuspensionSetupL, i);
	}

	return true;
}

void ATrackedVehicle::AnimateSprocketOrIdler(UStaticMeshComponent* SprocketOrIdlerComponent, float TrackAngVel, bool FlipAnimation180Degrees)
{
	float Mul;
	if (FlipAnimation180Degrees)
		Mul = 1.f;
	else
		Mul = -1.f;

	SprocketOrIdlerComponent->AddLocalRotation(FRotator(FMath::RadiansToDegrees(TrackAngVel)*GetWorld()->GetDeltaSeconds()*Mul, 0, 0));
}

void ATrackedVehicle::ShowSuspensionHandles(bool DebugMode)	// Debug
{

}

void ATrackedVehicle::SpawnDust(TArray < FSuspensionInternalProcessing> SuspensionSide, float TrackLinearVelSlide)
{
	if (FMath::RandRange(0.f, 1.f) > 0.25f)
	{
		for (FSuspensionInternalProcessing Susp : SuspensionSide)
		{
			if (FMath::RandRange(0.f, 1.f) > 0.5f)
			{
				// R = Actual*k, k = R/Actual
				float Norm;
				
				if (TrackLinearVelSlide >= 300)
					Norm = 1.f;
				else if (TrackLinearVelSlide > 0)
					Norm = 300 / TrackLinearVelSlide;
				else
					Norm = 0.f;

				if (FMath::RandRange(0.f, Norm) > 0.8f)
				{
					if (Susp.Engaged)
					{
						//UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), UParticleEmitter* , Susp.WheelCollisionLocation, true);
					}
				}
			}
		}
	}
}

void ATrackedVehicle::TraceForSuspension(FHitResult& OutHit, FVector Start, FVector End, float Radius)
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	
	UKismetSystemLibrary::SphereTraceSingle_NEW(GetWorld(), Start, End, Radius, UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility), false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, OutHit, true);
}

bool ATrackedVehicle::AnimateTreadsMaterial(float DeltaTime) // Dummy?
{
	TreadUVOffsetRight += (TrackRightLinVel*DeltaTime) / (TreadLength / TreadUvTiles);
	TreadMaterialRight->SetScalarParameterValue(FName("UVOffset"), TreadUVOffsetRight);

	TreadUVOffsetLeft += (TrackLeftLinVel*DeltaTime) / (TreadLength / TreadUvTiles);
	TreadMaterialLeft->SetScalarParameterValue(FName("UVOffset"), TreadUVOffsetLeft);

	return true;
}

// TODO Optimize Code
bool ATrackedVehicle::AnimateTreadsInstancedMesh(USplineComponent* SplineR, USplineComponent* SplineL, UInstancedStaticMeshComponent* TreadsR, UInstancedStaticMeshComponent* TreadsL)
{
	if (!SplineR || !SplineL || !TreadsR || !TreadsL)
	{
		return false;
	}

	TreadMeshOffsetRight = FMath::Fmod(TreadMeshOffsetRight + TrackRightLinVel*GetWorld()->GetDeltaSeconds(), SplineR->GetSplineLength());
	
	for (int i = 0; i <= TreadsLastIndex; i++)
	{
		// RightTread
		float RightTreadNum = (SplineR->GetSplineLength() / TreadsOnSide)*i + TreadMeshOffsetRight;
		float Remainder = FMath::Fmod(RightTreadNum, SplineR->GetSplineLength());
		float Distance;

		if (Remainder < 0.f)
		{
			Distance = SplineR->GetSplineLength() + Remainder;
		}
		else
		{
			Distance = Remainder;
		}

		FVector DistanceAlongSpline = SplineR->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		FRotator RotationAlongSpline = SplineR->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		FVector RightVectorAlongSpline = SplineR->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);

		float RollOffset = 0.f;

		if (RightVectorAlongSpline.Y < 0)
		{
			RollOffset = 180.f;
		}

		FRotator FinalRotation = FRotator(RotationAlongSpline.Pitch, RotationAlongSpline.Yaw, RotationAlongSpline.Roll + RollOffset);

		FTransform Transform = FTransform(FinalRotation, DistanceAlongSpline, FVector(1.f, 1.f, 1.f));

		TreadsR->UpdateInstanceTransform(i, Transform, false, TreadsLastIndex == i, false);
	}

	for (int i = 0; i <= TreadsLastIndex; i++)
	{
		// RightTread
		float RightTreadNum = (SplineL->GetSplineLength() / TreadsOnSide)*i + TreadMeshOffsetRight;
		float Remainder = FMath::Fmod(RightTreadNum, SplineL->GetSplineLength());
		float Distance;

		if (Remainder < 0.f)
		{
			Distance = SplineL->GetSplineLength() + Remainder;
		}
		else
		{
			Distance = Remainder;
		}

		FVector DistanceAlongSpline = SplineL->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		FRotator RotationAlongSpline = SplineL->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		FVector RightVectorAlongSpline = SplineL->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);

		float RollOffset = RotationAlongSpline.Roll;

		if (RightVectorAlongSpline.Y < 0)
		{
			RollOffset = 180.f;
		}

		FRotator FinalRotation = FRotator(RotationAlongSpline.Pitch, RotationAlongSpline.Yaw, RollOffset);

		FTransform Transform = FTransform(FinalRotation, DistanceAlongSpline, FVector(1.f, 1.f, 1.f));

		TreadsL->UpdateInstanceTransform(i, Transform, false, TreadsLastIndex == i, false);
	}
	
	return true;
}

// Dummy?
bool ATrackedVehicle::AnimateTreadsSplineControlPoints(UStaticMeshComponent* WheelMeshComponent, USplineComponent* TreadSplineComponent, int BottomCPIndex, int TopCPIndex, TArray<FVector>& SplineCoordinates, TArray<FSuspensionSetup> SuspensionSet, int SuspensionIndex)
{
	// Protect the arrays
	if (SuspensionSet.Num() < SuspensionIndex + 1)
	{
		UE_LOG(LogTemp, Error, TEXT("SuspensionSet not Set for %i!"), SuspensionIndex);
		return false;
	}

	if (SplineCoordinates.Num() < BottomCPIndex + 1)
	{
		UE_LOG(LogTemp, Error, TEXT("SplineCoordinates not set?"));
		return false;
	}

	if (SplineCoordinates.Num() < TopCPIndex + 1)
	{
		UE_LOG(LogTemp, Error, TEXT("SplineCoordinates not set?"));
		return false;
	}

	FVector WheelRelativeTransformLocation = WheelMeshComponent->GetRelativeTransform().GetLocation();
	FVector BottomCPSplineCoordinate = SplineCoordinates[BottomCPIndex];
	FVector TopCPSplineCoordinate = SplineCoordinates[TopCPIndex];
	FSuspensionSetup CurrentSetup = SuspensionSet[SuspensionIndex];

	FVector BottomCPSplineLocalLocation = FVector(WheelRelativeTransformLocation.X, BottomCPSplineCoordinate.Y, WheelRelativeTransformLocation.Z - CurrentSetup.CollisionRadius + TreadHalfThickness);

	FVector TopCPSplineLocalLocation = FVector(WheelRelativeTransformLocation.X, TopCPSplineCoordinate.Y, 0);

	if (WheelRelativeTransformLocation.Z + CurrentSetup.CollisionRadius - TreadHalfThickness > TopCPSplineCoordinate.Z)
	{
		TopCPSplineLocalLocation.Z = WheelRelativeTransformLocation.Z + CurrentSetup.CollisionRadius - TreadHalfThickness;
	}
	else
	{
		TopCPSplineLocalLocation.Z = TopCPSplineCoordinate.Z;
	}

	TreadSplineComponent->SetLocationAtSplinePoint(BottomCPIndex, BottomCPSplineLocalLocation, ESplineCoordinateSpace::Local, true);
	TreadSplineComponent->SetLocationAtSplinePoint(TopCPIndex, TopCPSplineLocalLocation, ESplineCoordinateSpace::Local, true);

	return false;
}

void ATrackedVehicle::ShiftGear(int ShiftUpOrDown)
{
	if (AutoGearBox)
	{
		int min, max;
		if (ReverseGear)
		{
			min = 0;
			max = NeutralGearIndex - 1;
		}
		else
		{
			min = NeutralGearIndex + 1;
			max = GearRatio.Num() - 1;
		}


		CurrentGear = FMath::Clamp<int>(CurrentGear + ShiftUpOrDown, min, max);
	}
	else
	{
		CurrentGear = FMath::Clamp<int>(CurrentGear + ShiftUpOrDown, 0, GearRatio.Num() - 1);

		ReverseGear = !(CurrentGear >= NeutralGearIndex);
	}
}

void ATrackedVehicle::UpdateAutoGearBox()
{
	float MinTime;
	float MaxTime;
	EngineTorqueCurve->GetTimeRange(MinTime, MaxTime);

	float MagicNumber = (EngineRPM - MinTime) / (MaxTime - MinTime);

	if (AutoGearBox && Throttle > 0.f)
	{
		if (AxleAngVel > LastAutoGearBoxAxleCheck)
		{
			if (MagicNumber >= GearUpShiftPrc)
			{
				if (ReverseGear)
					ShiftGear(-1);
				else
					ShiftGear(1);

				LastAutoGearBoxAxleCheck = AxleAngVel;
			}
		}
		else
		{
			if (MagicNumber < GearDownShiftPrc)
			{
				if (ReverseGear)
					ShiftGear(1);
				else
					ShiftGear(-1);

				LastAutoGearBoxAxleCheck = AxleAngVel;
			}
		}
	}
}

void ATrackedVehicle::SetRemoveAutoGearBox(bool ShouldSetTimer)
{
	if (ShouldSetTimer)
	{
		GetWorld()->GetTimerManager().SetTimer(AutoGearBoxTimerHandle, this, &ATrackedVehicle::UpdateAutoGearBox, 0.5f, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoGearBoxTimerHandle);
	}
}

void ATrackedVehicle::GetThrottleInputForAutoHandling(float InputVehicleLeftRight, float InputVehicleForwardBackward)
{
	float AxisInputValue = InputVehicleForwardBackward;
	FVector LocalVelocity = GetActorTransform().InverseTransformVector(GetVelocity());
	bool ForwardBackwardPressed = InputVehicleForwardBackward != 0;
	bool IsVehicleMoving = LocalVelocity.Size() > 10;
	bool IsForwardPressed = FMath::Sign(AxisInputValue) > 0.f;
	bool IsBackwardPressed = FMath::Sign(AxisInputValue) < 0.f;
	bool ForwardMovementWithForwardPressed = FMath::Sign(LocalVelocity.X) > 0.f && IsForwardPressed;
	bool AreWeSteeringWithNoMovement = InputVehicleLeftRight != 0.f && !(IsVehicleMoving);
	
	//UE_LOG(LogTemp, Warning, TEXT("AxisInputValue: %f, ForwardBackwardPressed: %i, IsVehicleMoving: %i, IsForwardPressed: %i, ForwardMovementPressed: %i, AreWeSteeringStill: %i"), AxisInputValue, (int)ForwardBackwardPressed, (int)IsVehicleMoving, (int)IsForwardPressed, (int)ForwardMovementWithForwardPressed, (int)AreWeSteeringWithNoMovement);
	//UE_LOG(LogTemp, Warning, TEXT("WheelRightCoef: %f, WheelLeftCoef: %f"), WheelRightCoefficient, WheelLeftCoefficient);

	UE_LOG(LogTemp, Warning, TEXT("Right AngularVelocity: %f, Left AngularVelocity: %f"), TrackRightAngVel, TrackLeftAngVel);

	if (ForwardBackwardPressed)
	{
		if (IsVehicleMoving)
		{
			if (IsForwardPressed)
			{
				if (ForwardMovementWithForwardPressed)
				{
					ReverseGear = false;
					ShiftGear(0);

					BrakeRatioRight = 0.f;
					BrakeRatioLeft = 0.f;
					WheelForwardCoefficient = FMath::Abs(AxisInputValue);
				}
				else
				{
					float BrakeRatio = FMath::Abs(AxisInputValue);
					BrakeRatioRight = BrakeRatio;
					BrakeRatioLeft = BrakeRatio;

					WheelForwardCoefficient = 0.f;
				}
			}
			else
			{
				if (ForwardMovementWithForwardPressed)
				{
					float BrakeRatio = FMath::Abs(AxisInputValue);
					BrakeRatioRight = BrakeRatio;
					BrakeRatioLeft = BrakeRatio;

					WheelForwardCoefficient = 0.f;
				}
				else
				{
					ReverseGear = true;

					ShiftGear(0);

					BrakeRatioRight = 0.f;
					BrakeRatioLeft = 0.f;
					WheelForwardCoefficient = FMath::Abs(AxisInputValue);
				}
			}
		}
		else
		{
			if (IsForwardPressed)
			{
				ReverseGear = false;
				ShiftGear(0);

				BrakeRatioRight = 0.f;
				BrakeRatioLeft = 0.f;
				WheelForwardCoefficient = FMath::Abs(AxisInputValue);
			}
			if (IsBackwardPressed)
			{
				ReverseGear = true;
				ShiftGear(0);

				BrakeRatioRight = 0.f;
				BrakeRatioLeft = 0.f;
				WheelForwardCoefficient = FMath::Abs(AxisInputValue);
			}
		}
	}
	else
	{
		if (AreWeSteeringWithNoMovement)
		{
			ReverseGear = false;
			ShiftGear(0);

			BrakeRatioRight = 0.f;
			BrakeRatioLeft = 0.f;
			WheelForwardCoefficient = FMath::Abs(AxisInputValue);
		}
		else
		{
			BrakeRatioRight = 0.f;
			BrakeRatioLeft = 0.f;
			WheelForwardCoefficient = FMath::Abs(AxisInputValue);
		}
	}
}

void ATrackedVehicle::GetGearBoxInfo(int& GearNum, bool& ReverseGear, bool& Automatic)
{
	if (ReverseGear)
	{
		GearNum = NeutralGearIndex - CurrentGear;
	}
	else
	{
		GearNum = CurrentGear - NeutralGearIndex;
	}

	ReverseGear = this->ReverseGear;
	Automatic = AutoGearBox;
}

void ATrackedVehicle::GetMuFromFrictionElipse(float& Mu_Static, float& Mu_Kinetic, FVector VelocityDirectionNormalized, FVector ForwardVector, float Mu_X_StaticL, float Mu_Y_StaticL, float Mu_X_KineticL, float Mu_Y_KineticL)
{
	float DotProd = FVector::DotProduct(VelocityDirectionNormalized, ForwardVector);

	float YComp = FMath::Sqrt(1 - DotProd*DotProd);

	Mu_Static = FVector2D(Mu_X_StaticL*DotProd, Mu_Y_StaticL*YComp).Size();
	Mu_Kinetic = FVector2D(Mu_X_KineticL*DotProd, Mu_Y_KineticL*YComp).Size();
}