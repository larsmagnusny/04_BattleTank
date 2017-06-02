// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "TrackedVehicle.h"
#include "Carriage.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
ATrackedVehicle::ATrackedVehicle()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATrackedVehicle::BeginPlay()
{
	Super::BeginPlay();
	
	SetRemoveAutoGearBox(true);
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
		UpdateThrottle();
		UpdateWheelsVelocity(DeltaTime);
		AnimateWheels();
		AnimateTreadsMaterial();
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

		ApplyDriveForceAndGetFrictionForceOnSide(this->TrackFrictionTorqueRight, this->TrackRollingFrictionTorqueRight, SuspensionsInternalRight, this->DriveRightForce, this->TrackRightLinVel);
		ApplyDriveForceAndGetFrictionForceOnSide(this->TrackFrictionTorqueLeft, this->TrackRollingFrictionTorqueLeft, SuspensionsInternalLeft, this->DriveLeftForce, this->TrackLeftLinVel);

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

	InputComponent->BindAxis(FName("FWD"), this, &ATrackedVehicle::ForwardBackward);
}

void ATrackedVehicle::ForwardBackward(float AxisValue)
{
	//UE_LOG(LogTemp, Warning, TEXT("ForwardBackward!!"));
	if (AutoGearBox)
	{
		GetThrottleInputForAutoHandling(AxisValue, 0.f);
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

	if (AxisValue != 0)
	{
		SleepMod = false;
		SleepDelayTimer = 0.f;
	}
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

	bool BlockingHit = false;
	FVector Location;
	FVector ImpactPoint;
	FVector ImpactNormal;
	EPhysicalSurface SurfaceType;
	UPrimitiveComponent* CollisionPrimitive = nullptr;

	// TODO check if this is the reason for wonky physics...
	TraceForSuspension(BlockingHit, Location, ImpactPoint, ImpactNormal, SurfaceType, CollisionPrimitive, SuspWorldLocation, SuspWorldLocation + SuspWorldZVector*(-1.f)*SuspLength, SuspensionArray[SuspIndex].Radius);

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
	FVector CenterOfMass = Body->GetCenterOfMass();

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
	FVector ResVel = ActorTransform.InverseTransformVector(Body->GetPhysicsLinearVelocity()) + FVector::CrossProduct(ActorTransform.InverseTransformVector(Body->GetPhysicsAngularVelocity())*(0.01745329252f), (ActorTransform.InverseTransformPosition(PointLoc) - ActorTransform.InverseTransformPosition(Body->GetCenterOfMass())));

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

void ATrackedVehicle::UpdateThrottle()
{
	TrackTorqueTransferRight = FMath::Clamp<float>(WheelForwardCoefficient + WheelRightCoefficient, -1.f, 2.f);
	TrackTorqueTransferLeft = FMath::Clamp<float>(WheelForwardCoefficient + WheelLeftCoefficient, -1.f, 2.f);

	if (FMath::Max<float>(FMath::Abs<float>(TrackTorqueTransferRight), FMath::Abs<float>(TrackTorqueTransferLeft)) != 0.f)
	{
		ThrottleIncrement = 0.f;
	}
	else
	{
		ThrottleIncrement = -1.f;
	}

	Throttle = FMath::Clamp<float>(ThrottleIncrement*GetWorld()->GetDeltaSeconds() + Throttle, 0.f, 1.f);
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
	EngineTorque = GetEngineTorque(GetEngineRPMFromAxle(AxleAngVel))*Throttle;

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

void ATrackedVehicle::ApplyDriveForceAndGetFrictionForceOnSide(float& TotalFrictionTorqueSide, float& TotalRollingFrictionTorqueSide, TArray<FSuspensionInternalProcessing> SuspensionSide, FVector DriveForceSide, float TrackLinearVelSide)
{
	float TotalTrackFrictionTorque = 0.f, TrackFrictionTorque = 0.f, VehicleMass = 0.f, WheelLoadN = 0.f, TrackRollingFrictionTorque = 0.f, TotalTrackRollingFrictionTorque = 0.f, MuStatic = 0.f, MuKinetic = 0.f;
	FVector RelativeTrackVel, FullStaticFrictionForce, FullStaticDriveForce, ApplicationForce, RollingFriction, FullDriveForceNorm, FullFrictionForceNorm, FrictionForceX, FrictionForceY, FullKineticFrictionForce, FullKineticDriveForce;
	for (FSuspensionInternalProcessing Susp : SuspensionSide)
	{
		if (Susp.Engaged)
		{
			WheelLoadN = Susp.SuspensionForce.ProjectOnTo(Susp.WheelCollisionNormal).Size();

			FVector SlideVel = (GetVelocityAtPointWorld(Susp.WheelCollisionLocation) - GetActorForwardVector()*TrackLinearVelSide);
			RelativeTrackVel = FVector::VectorPlaneProject(SlideVel, Susp.WheelCollisionNormal);

			// TODO check if this works...
			GetMuFromFrictionElipse(MuStatic, MuKinetic, RelativeTrackVel.GetSafeNormal(), GetActorForwardVector(), Mu_X_Static, Mu_Y_Static, Mu_X_Kinetic, Mu_Y_Kinetic);

			VehicleMass = GetVehicleMass();

			FVector UnProjectedSlide = RelativeTrackVel*(-1)*VehicleMass / GetWorld()->GetDeltaSeconds() / TotNumFrictionPoints;

			FVector ProjectedForwardVector = FVector::VectorPlaneProject(GetActorForwardVector(), Susp.WheelCollisionNormal).GetSafeNormal();
			FVector ProjectedRightVector = FVector::VectorPlaneProject(GetActorRightVector(), Susp.WheelCollisionNormal).GetSafeNormal();

			FVector a = UnProjectedSlide.ProjectOnTo(ProjectedForwardVector);
			FVector b = UnProjectedSlide.ProjectOnTo(ProjectedRightVector);

			FullStaticFrictionForce = a*Mu_X_Static + b*Mu_Y_Static;
			FullKineticFrictionForce = a*Mu_X_Kinetic + b*Mu_Y_Kinetic;

			FVector DriveForceTransmissionTorque = FVector::VectorPlaneProject(DriveForceSide, Susp.WheelCollisionNormal);

			FullStaticDriveForce = DriveForceTransmissionTorque*Mu_X_Static;
			FullKineticDriveForce = DriveForceTransmissionTorque*Mu_X_Kinetic;

			if ((FullStaticFrictionForce + FullStaticDriveForce).Size() >= WheelLoadN*MuStatic)
			{
				FullFrictionForceNorm = FullKineticFrictionForce.GetSafeNormal();
				ApplicationForce = (FullKineticFrictionForce + FullKineticDriveForce).ClampSize(0, WheelLoadN*MuKinetic);

				DrawDebugString(GetWorld(), Susp.WheelCollisionLocation, "KineticFriction", nullptr, FLinearColor(1.f, 0.041f, 0.415f).ToFColor(true), 0.f);
			}
			else
			{
				FullFrictionForceNorm = FullStaticFrictionForce.GetSafeNormal();
				ApplicationForce = (FullStaticFrictionForce + FullStaticDriveForce).ClampSize(0, WheelLoadN*MuStatic);

				DrawDebugString(GetWorld(), Susp.WheelCollisionLocation, "StaticFriction", nullptr, FLinearColor(1.f, 0.057f, 0.f).ToFColor(true), 0.f);
			}

			Body->AddForceAtLocation(ApplicationForce, Susp.WheelCollisionLocation);

			DrawDebugLine(GetWorld(), Susp.WheelCollisionLocation, Susp.WheelCollisionLocation + ApplicationForce*0.001f, FLinearColor(0.f, 0.5f, 0.5f).ToFColor(true), false, -1.f, 0, 10);

			TrackFrictionTorque = GetActorTransform().TransformVector((ApplicationForce.ProjectOnTo(FullFrictionForceNorm) / VehicleMass)*(TrackMassKg + SprocketMassKg)*(-1)).ProjectOnTo(FVector(1.f, 0.f, 0.f)).X * SprocketRadiusCm;
			
			// Tracklinearvelslide
			TrackRollingFrictionTorque = WheelLoadN*RollingFrictionCoef*FMath::Sign(TrackLinearVelSide)*(-1) + FMath::Sign(TrackLinearVelSide)*FMath::Abs(TrackLinearVelSide)*0.000015f*WheelLoadN;
		
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
	float NewVel;
	if (FMath::Abs<float>(AngVel_In) > FMath::Abs<float>(BrakeRatio*BrakeForce*DeltaTime))
	{
		NewVel = AngVel_In - (BrakeRatio*BrakeForce*DeltaTime*FMath::Sign(AngVel_In));
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

void ATrackedVehicle::TraceForSuspension(bool& BlockingHit, FVector& Location, FVector& ImpactPoint, FVector& ImpactNormal, EPhysicalSurface& SurfaceType, UPrimitiveComponent* Component, FVector Start, FVector End, float Radius)
{
	FHitResult Hit;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	
	if (UKismetSystemLibrary::SphereTraceSingle_NEW(GetWorld(), Start, End, Radius, UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility), false, ActorsToIgnore, EDrawDebugTrace::ForOneFrame, Hit, true))
	{
		BlockingHit = Hit.bBlockingHit;
		Location = Hit.Location;
		ImpactPoint = Hit.ImpactPoint;
		ImpactNormal = Hit.ImpactNormal;
		Component = Hit.GetComponent();

		if (Hit.PhysMaterial.Get() != nullptr)
		{
			SurfaceType = Hit.PhysMaterial.Get()->SurfaceType;
		}
	}
	else
	{
		BlockingHit = false;
	}
}

bool ATrackedVehicle::AnimateTreadsMaterial() // Dummy?
{
	TreadUVOffsetRight += (TrackRightLinVel*GetWorld()->GetDeltaSeconds()) / (TreadLength / TreadUvTiles);
	TreadMaterialRight->GetScalarParameterValue(FName("UVOffset"), TreadUVOffsetRight);
	TreadUVOffsetLeft += (TrackLeftLinVel*GetWorld()->GetDeltaSeconds()) / (TreadLength / TreadUvTiles);
	TreadMaterialLeft->GetScalarParameterValue(FName("UVOffset"), TreadUVOffsetLeft);
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

		// Left Tread

		float LeftTreadNum = (SplineL->GetSplineLength() / TreadsOnSide)*i + TreadMeshOffsetLeft;
		Remainder = FMath::Fmod(LeftTreadNum, SplineL->GetSplineLength());
		Distance;

		if (Remainder < 0.f)
		{
			Distance = SplineL->GetSplineLength() + Remainder;
		}
		else
		{
			Distance = Remainder;
		}

		DistanceAlongSpline = SplineL->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		RotationAlongSpline = SplineL->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		RightVectorAlongSpline = SplineL->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);

		RollOffset = 0.f;

		if (RightVectorAlongSpline.Y < 0)
		{
			RollOffset = 180.f;
		}

		FinalRotation = FRotator(RotationAlongSpline.Pitch, RotationAlongSpline.Yaw, RotationAlongSpline.Roll + RollOffset);

		Transform = FTransform(FinalRotation, DistanceAlongSpline, FVector(1.f, 1.f, 1.f));

		TreadsL->UpdateInstanceTransform(i, Transform, false, TreadsLastIndex == i, false);
	}
	
	return true;
}

// Dummy?
bool ATrackedVehicle::AnimateTreadsSplineControlPoints(UStaticMeshComponent* WheelMeshComponent, USplineComponent* TreadSplineComponent, int BottomCPIndex, int TopCPIndex, TArray<FVector>& SplineCoordinates, TArray<FSuspensionSetup> SuspensionSet, int SuspensionIndex)
{
	if (SuspensionSet.Num() < SuspensionIndex + 1)
		return false;

	FTransform WheelTransform = WheelMeshComponent->GetRelativeTransform();

	FVector Location = FVector(WheelTransform.GetLocation().X, SplineCoordinates[BottomCPIndex].Y, (WheelTransform.GetLocation().Z - SuspensionSet[SuspensionIndex].CollisionRadius)*TreadHalfThickness);
	float Z = (WheelTransform.GetLocation().Z + SuspensionSet[SuspensionIndex].CollisionRadius)*TreadHalfThickness;

	if (Z <= SplineCoordinates[TopCPIndex].Z)
	{
		Z = SplineCoordinates[TopCPIndex].Z;
	}


	FVector Location2 = FVector(WheelTransform.GetLocation().X, SplineCoordinates[TopCPIndex].Y, Z);

	TreadSplineComponent->SetLocationAtSplinePoint(BottomCPIndex, Location, ESplineCoordinateSpace::Local, true);
	
	return false;
}

void ATrackedVehicle::ShiftGear(int ShiftUpOrDown)
{
	if (AutoGearBox)
	{
		int minGear = NeutralGearIndex + 1;
		int maxGear = 7;

		if (ReverseGear)
		{
			minGear = 0;
			maxGear = NeutralGearIndex - 1;
		}

		CurrentGear = FMath::Clamp<int>(CurrentGear + ShiftUpOrDown, minGear, maxGear);
	}
	else
	{
		CurrentGear = FMath::Clamp<int>(CurrentGear + ShiftUpOrDown, 0, 7);

		ReverseGear = !(CurrentGear >= NeutralGearIndex);
	}
}

void ATrackedVehicle::UpdateAutoGearBox()
{
	if (AutoGearBox && Throttle > 0.f)
	{
		if (AxleAngVel > LastAutoGearBoxAxleCheck)
		{
			float MaxTime, MinTime;
			EngineTorqueCurve->GetTimeRange(MinTime, MaxTime);

			if ((EngineRPM - MinTime) / (MaxTime - MinTime) >= GearUpShiftPrc)
			{
				int ShiftUpDown = 1;
				if (ReverseGear)
					ShiftUpDown = -1;
				ShiftGear(ShiftUpDown);
				LastAutoGearBoxAxleCheck = AxleAngVel;
			}
		}
		if (AxleAngVel < LastAutoGearBoxAxleCheck)
		{
			float MaxTime, MinTime;
			EngineTorqueCurve->GetTimeRange(MinTime, MaxTime);

			if ((EngineRPM - MinTime) / (MaxTime - MinTime) < GearDownShiftPrc)
			{
				int ShiftUpDown = -1;
				if (ReverseGear)
					ShiftUpDown = 1;
				ShiftGear(ShiftUpDown);
				LastAutoGearBoxAxleCheck = AxleAngVel;
			}
		}
	}
}

void ATrackedVehicle::SetRemoveAutoGearBox(bool ShouldSetTimer)
{
	if (ShouldSetTimer)
	{
		GetWorld()->GetTimerManager().SetTimer(AutoGearBoxTimerHandle, this, &ATrackedVehicle::UpdateAutoGearBox, false);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoGearBoxTimerHandle);
	}
}

void ATrackedVehicle::GetThrottleInputForAutoHandling(float InputVehicleLeftRight, float InputVehicleForwardBackward)
{
	float AxisInputValue = InputVehicleForwardBackward;
	FTransform ActorTransform = GetActorTransform();
	FVector Velocity = ActorTransform.InverseTransformVector(GetVelocity());

	// Forward/backward is not pressed
	if (AxisInputValue != 0.f)
	{
		// Are we moving?
		if (Velocity.Size() > 10)
		{
			// Is forward pressed?
			if (FMath::Sign<float>(AxisInputValue) > 0.f)
			{
				// We are moving forward with forward pressed
				if (FMath::Sign<float>(ActorTransform.GetLocation().X) > 0)
				{
					// Forward
					ReverseGear = false;
					ShiftGear(0);
					BrakeRatioRight = 0.f;
					BrakeRatioLeft = 0.f;
					WheelForwardCoefficient = FMath::Abs(AxisInputValue);
				}
				else
				{
					// Brake
					BrakeRatioRight = FMath::Abs(AxisInputValue);
					BrakeRatioLeft = FMath::Abs(AxisInputValue);

					WheelForwardCoefficient = 0.f;
				}
			}
			else
			{
				// We are moving forward with backward pressed
				if (FMath::Sign<float>(ActorTransform.GetLocation().X) > 0)
				{
					// Brake
					BrakeRatioRight = FMath::Abs(AxisInputValue);
					BrakeRatioLeft = FMath::Abs(AxisInputValue);

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
			// Not moving yet but forward/backward is pressed
			if (FMath::Sign(AxisInputValue) > 0.f)
			{
				// Forward
				ReverseGear = false;
				ShiftGear(0);

				ShiftGear(0);
				BrakeRatioRight = 0.f;
				BrakeRatioLeft = 0.f;
				WheelForwardCoefficient = FMath::Abs(AxisInputValue);
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
		// No throttle but maybe we are steering
		if (InputVehicleLeftRight != 0.f && !(Velocity.Size() > 10))
		{
			// We are steering without throttle and not rolling
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