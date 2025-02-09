// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetLock/GAS/Tasks/GASTask_TargetLock.h"
#include "TargetLockUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

UGASTask_TargetLock::UGASTask_TargetLock(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
}

UGASTask_TargetLock* UGASTask_TargetLock::StartTargetLock(UGameplayAbility* OwningAbility, FName TaskInstanceName, FStruct_TargetLockData TaskData, AActor* OptionalOwningActor, UCameraComponent* OptionalCamera)
{
	UGASTask_TargetLock* MyObj = NewAbilityTask<UGASTask_TargetLock>(OwningAbility, TaskInstanceName);
	
	MyObj->Configuration = TaskData;
	
	MyObj->SetupTargetLock(OptionalCamera, OptionalOwningActor);

	if (!MyObj->CameraLockTarget)
	{
		MyObj->ConditionalBeginDestroy();
		return nullptr;
	}
	
	return MyObj;
}

void UGASTask_TargetLock::Activate()
{
	Super::Activate();
	
	if (CameraLockTarget && Configuration.TargetLockVisualizeActorClass)
	{
		TargetLockVisualizeActor = GetWorld()->SpawnActor(Configuration.TargetLockVisualizeActorClass);
		TargetLockVisualizeActor->AttachToActor(CameraLockTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	if (!CameraLockTarget)
	{
		StopTask_Implementation();
	}
}

void UGASTask_TargetLock::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
	if (TargetLockVisualizeActor)
	{
		TargetLockVisualizeActor->Destroy();
	}
}

void UGASTask_TargetLock::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	LerpTargetLocked(DeltaTime);
}

void UGASTask_TargetLock::SetupTargetLock(UCameraComponent* OptionalCam, AActor* OptionalOwner)
{
	
	AActor* OwningActor = nullptr;
	if (OptionalOwner)
	{
		OwningActor = OptionalOwner;
	}
	else
	{
		OwningActor = GetOwnerActor();
	}

	if (OptionalCam)
	{
		CameraComponent = OptionalCam;
	}
	else
	{
		CameraComponent = Cast<UCameraComponent>(OwningActor->GetComponentByClass(UCameraComponent::StaticClass()));
	}
	
	FVector CameraLocation = CameraComponent->GetComponentLocation();
	FVector CameraForward = CameraComponent->GetForwardVector();

	//Setup possible targets
	TArray<AActor*> PossibleTargets{};
	TArray<AActor*> TempTargets{};
	for (TSubclassOf<AActor> LockClass : Configuration.LockableClasses)
	{
		UKismetSystemLibrary::SphereOverlapActors(this,
			OwningActor->GetActorLocation(),
			Configuration.MaxDistanceToStartTargetLock,
			{ UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic), UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic) },
			LockClass,
			{},
			TempTargets);

		if (TempTargets.Num() > 0)
		{
			PossibleTargets.Append(TempTargets);
		}
		TempTargets.Empty();
	}
	

	//Find best target
	AActor* Target = nullptr;
	float ClosestTarget = FLT_MAX;
	for (AActor* Actor : PossibleTargets)
	{
		float dist = FVector::Dist(CameraLocation, Actor->GetActorLocation());
		if (dist >= ClosestTarget && dist > Configuration.MaxDistanceToStartTargetLock) continue;

		FVector Direction = Actor->GetActorLocation() - CameraLocation;
		float Angle = UTargetLockUtilities::GetAngleToDirection(CameraForward, Direction);
		if (Angle > Configuration.MaxAngleToTarget) continue;

		if (dist < Configuration.MaxDistanceToStartTargetLock && dist < ClosestTarget)
		{
			const TArray<AActor*> IgnoreList{ Actor, OwningActor };
			
			if(Configuration.DoLineOfSightCheck && !(UTargetLockUtilities::LineOfSightCheckFromCompToActor(this, CameraComponent, Actor, IgnoreList, 75) ||
				UTargetLockUtilities::LineOfSightCheckFromActorToActor(this, OwningActor, Actor, IgnoreList, 75)))
				continue;

			ClosestTarget = dist;
			Target = Actor;
		}
	}

	//Apply Lock Target, if this is still null here it will end the task at the start of the first tick
	CameraLockTarget = Target;
}

void UGASTask_TargetLock::LerpTargetLocked(double DeltaTime)
{
	//Check if Data is Valid
	if (!CameraLockTarget || !CameraComponent)
	{
		StopTask_Implementation();
		return;
	}

	//Do a Line of Sight Check, if required
	if (Configuration.ContinuousLineOfSightCheck)
	{
		if (!UTargetLockUtilities::LineOfSightCheckFromCompToActor(this,
			Cast<USceneComponent>(this), CameraLockTarget,
			{ CameraComponent->GetOwner(), CameraLockTarget }, 75) &&
			!UTargetLockUtilities::LineOfSightCheckFromActorToActor(this,
				CameraComponent->GetOwner(), CameraLockTarget,
				{ CameraComponent->GetOwner(), CameraLockTarget }, 75))
		{
			StopTask_Implementation();
			return;
		}
	}

	//Setup Calculation Data
	const FVector CameraLocation = CameraComponent->GetComponentLocation();
	const FVector TargetLocation = CameraLockTarget->GetActorLocation();
	const FVector CameraDirection = CameraComponent->GetForwardVector() * FVector::Dist(CameraLocation, TargetLocation);
	const FVector TargetDirection = TargetLocation - CameraLocation;

	//this vector is perpendicular to TargetLocation
	const FVector ProjectedVector = UKismetMathLibrary::ProjectVectorOnToVector(TargetDirection, CameraDirection);

	const float Angle = UTargetLockUtilities::GetAngleToDirection(TargetDirection, CameraDirection);

	//Early Return if we don't need any additional rotation
	if (Angle < Configuration.AngleToStartLerp)
	{
		return;
	}

	//Stop Target Lock if Target is outside range
	if (FVector::Dist(CameraComponent->GetComponentLocation(), CameraLockTarget->GetActorLocation()) > Configuration.MaxDistanceToStartTargetLock)
	{
		StopTask_Implementation();
		return;
	}

	//Create a Look At Target based on the projected vector to rotate camera towards
	FVector NormalizedAngledDirection = TargetLocation - (ProjectedVector + CameraLocation);
	NormalizedAngledDirection.Normalize();

	float DistFromProjectedToTarget = FVector::Dist(CameraLocation + (FVector::DownVector * 100) + ProjectedVector, TargetLocation);

	const FVector SoftRotationLookAtTarget = (NormalizedAngledDirection * (asin((Angle - Configuration.AngleToStartLerp) * (PI / 180)) * TargetDirection.Length()));
	const FVector HardRotationLookAtTarget = (NormalizedAngledDirection * (asin((Angle - Configuration.MaxAngleToTarget) * (PI / 180)) * TargetDirection.Length()));

	//These are the desired points based on our rotation to the target. 
	const FRotator TargetSoftRotator = UKismetMathLibrary::FindLookAtRotation(CameraLocation, CameraLocation + ProjectedVector + SoftRotationLookAtTarget);
	const FRotator TargetHardRotator = UKismetMathLibrary::FindLookAtRotation(CameraLocation, CameraLocation + ProjectedVector + HardRotationLookAtTarget);

	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);

	//Do the hard rotation, if needed based on our look at angle to the target
	if (Angle >= Configuration.MaxAngleToTarget)
	{
		const float TargetYaw = TargetHardRotator.Yaw;
		float ControllerYaw = Controller->GetControlRotation().Yaw;
		float CalcYaw = UTargetLockUtilities::FindRotationAddition(TargetYaw, ControllerYaw);
		CalcYaw *= DeltaTime * Configuration.HardRotateSpeedMultiplier;
		
		const float TargetPitch = TargetHardRotator.Pitch;
		float ControllerPitch = Controller->GetControlRotation().Pitch;
		float CalcPitch = UTargetLockUtilities::FindRotationAddition(TargetPitch, ControllerPitch);
		CalcPitch *= DeltaTime * Configuration.HardRotateSpeedMultiplier;

		//Forcefully rotate Camera
		Controller->SetControlRotation(Controller->GetControlRotation() + FRotator(CalcPitch, CalcYaw, 0));
		
	}
	
	//Do the smooth rotation towards the target
	if (Angle >= Configuration.AngleToStartLerp)
	{
		//Make smooth yaw input
		const float TargetYaw = TargetSoftRotator.Yaw;
		float ControllerYaw = Controller->GetControlRotation().Yaw;
		float CalcYaw = UTargetLockUtilities::FindRotationAddition(TargetYaw, ControllerYaw);
		CalcYaw *= Configuration.RotateSpeed * DeltaTime;
		
		//make smooth pitch input
		const float TargetPitch = TargetSoftRotator.Pitch;
		float ControllerPitch = Controller->GetControlRotation().Pitch;
		float CalcPitch = UTargetLockUtilities::FindRotationAddition(TargetPitch, ControllerPitch);
		CalcPitch *= Configuration.RotateSpeed * DeltaTime;

		Controller->SetControlRotation(Controller->GetControlRotation() + FRotator(CalcPitch, CalcYaw, 0));
	}
}

bool UGASTask_TargetLock::IsLockingOnTarget() const
{
	return static_cast<bool>(CameraLockTarget);
}

void UGASTask_TargetLock::StopTask_Implementation()
{
	if (TargetLockVisualizeActor)
	{
		TargetLockVisualizeActor->Destroy();
	}
	CameraLockTarget = nullptr;
	CameraComponent = nullptr;
	OnTaskEnded.Broadcast();
	EndTask();
}