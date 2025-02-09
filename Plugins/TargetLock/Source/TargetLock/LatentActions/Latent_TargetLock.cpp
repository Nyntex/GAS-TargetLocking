// Fill out your copyright notice in the Description page of Project Settings.


#include "Latent_TargetLock.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#define LATENT_RESPONSE_INFO LatentActionInfo.ExecutionFunction, LatentActionInfo.Linkage, LatentActionInfo.CallbackTarget

void ULatent_TargetLock::LatentTargetLock(UObject* WorldContext, FLatentActionInfo LatentInfo,
                                          ETargetLockInputPins InputPins, ETargetLockOutputPins& OutputPins, UCameraComponent* CameraComponent, TArray<TSubclassOf<AActor>> LockableClasses,
                                          AActor* LockTarget, float MaxAngleToTarget, float AngleToStartLerp, float RotateSpeed, float HardRotateSpeedMultiplier,
                                          float MaxDistanceToStartTargetLock, bool DoLineOfSightCheck, bool ContinuousLineOfSightCheck)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);

	if(!World)
	{
		if (WorldContext)
			World = WorldContext->GetWorld();
	}

	if(!World)
	{
		return;
	}

	FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
	FLatentTargetLock* ExistingAction = LatentActionManager.FindExistingAction<FLatentTargetLock>(LatentInfo.CallbackTarget, LatentInfo.UUID);

	if(InputPins == ETargetLockInputPins::Start)
	{
		if(!ExistingAction)
		{
			FLatentTargetLock* Action = new FLatentTargetLock(LatentInfo, OutputPins, CameraComponent, LockableClasses, LockTarget, MaxAngleToTarget, AngleToStartLerp,
				RotateSpeed, HardRotateSpeedMultiplier, MaxDistanceToStartTargetLock, DoLineOfSightCheck, ContinuousLineOfSightCheck);
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, Action);
		}
	}
	else if(InputPins == ETargetLockInputPins::Cancel)
	{
		if(ExistingAction)
		{
			ExistingAction->StopTargetLock();
		}
	}


}

void FLatentTargetLock::UpdateOperation(FLatentResponse& Response)
{
	//FPendingLatentAction::UpdateOperation(Response);

	LerpTargetLocked(Response);
}

void FLatentTargetLock::LerpTargetLocked(FLatentResponse& Response)
{
	if (!CameraLockTarget || !CameraComponent)
	{
		CancelTargetLock(Response);
		return;
	}

	if (ContinuousLineOfSightCheck)
	{
		if (!UTargetLockUtilities::LineOfSightCheckFromCompToActor(CameraComponent,
			Cast<USceneComponent>(this), CameraLockTarget,
			{ CameraComponent->GetOwner(), CameraLockTarget }, 75) &&
			!UTargetLockUtilities::LineOfSightCheckFromActorToActor(CameraComponent,
				CameraComponent->GetOwner(), CameraLockTarget,
				{ CameraComponent->GetOwner(), CameraLockTarget }, 75))
		{
			CancelTargetLock(Response);
			return;
		}
	}
	
	//we clamp the value to be 0.1 (100 fps) in order to keep uncontrollable spins from happening
	float DeltaTime = FMath::Min(Response.ElapsedTime(), 0.1f);

	const FVector CameraLocation = CameraComponent->GetComponentLocation();
	const FVector TargetLocation = CameraLockTarget->GetActorLocation();
	const FVector CameraDirection = CameraComponent->GetForwardVector() * FVector::Dist(CameraLocation, TargetLocation);
	const FVector TargetDirection = TargetLocation - CameraLocation;

	//this vector is perpendicular to TargetLocation
	const FVector ProjectedVector = UKismetMathLibrary::ProjectVectorOnToVector(TargetDirection, CameraDirection);

	const float Angle = UTargetLockUtilities::GetAngleToDirection(TargetDirection, CameraDirection);

	//stop execution when the angle is already ok
	if (Angle < AngleToStartLerp)
	{
		UpdateTargetLock(Response);
		return;
	}

	//Create a Look At Target based on the projected vector to rotate camera towards
	FVector NormalizedAngledDirection = TargetLocation - (ProjectedVector + CameraLocation);
	NormalizedAngledDirection.Normalize();

	const FVector SoftRotationLookAtTarget = (NormalizedAngledDirection * (asin((Angle - AngleToStartLerp) * (PI / 180)) * TargetDirection.Length()));
	const FVector HardRotationLookAtTarget = (NormalizedAngledDirection * (asin((Angle - MaxAngleToTarget) * (PI / 180)) * TargetDirection.Length()));

	const FRotator TargetSoftRotator = UKismetMathLibrary::FindLookAtRotation(CameraLocation, CameraLocation + ProjectedVector + SoftRotationLookAtTarget);
	const FRotator TargetHardRotator = UKismetMathLibrary::FindLookAtRotation(CameraLocation, CameraLocation + ProjectedVector + HardRotationLookAtTarget);

	APlayerController* Controller = UGameplayStatics::GetPlayerController(CameraLockTarget, 0);

	if (Angle >= MaxAngleToTarget)
	{

		const float TargetYaw = TargetSoftRotator.Yaw;
		float ControllerYaw = Controller->GetControlRotation().Yaw;
		float CalcYaw = TargetHardRotator.Yaw - ControllerYaw;
		CalcYaw *= DeltaTime * HardRotateSpeedMultiplier;


		const float TargetPitch = TargetSoftRotator.Pitch;
		float ControllerPitch = Controller->GetControlRotation().Pitch;
		float CalcPitch = TargetHardRotator.Pitch - ControllerPitch;
		CalcPitch *= -DeltaTime * HardRotateSpeedMultiplier;
		Controller->SetControlRotation(FRotator(-CalcPitch, CalcYaw, 0) + Controller->GetControlRotation());
	}

	if (Angle >= AngleToStartLerp) //This -0.5 is to be able to rotate the camera
	{

		//Make smooth yaw input
		const float TargetYaw = TargetSoftRotator.Yaw;
		float ControllerYaw = Controller->GetControlRotation().Yaw;
		float CalcYaw = (TargetYaw)-ControllerYaw;
		CalcYaw *= RotateSpeed * DeltaTime;


		//make smooth pitch input
		const float TargetPitch = TargetSoftRotator.Pitch;
		float ControllerPitch = Controller->GetControlRotation().Pitch;
		float CalcPitch = TargetSoftRotator.Pitch - ControllerPitch;
		CalcPitch *= -RotateSpeed * DeltaTime;

		Controller->SetControlRotation(FRotator(-CalcPitch, CalcYaw, 0) + Controller->GetControlRotation());
	}

	UpdateTargetLock(Response);
}

void FLatentTargetLock::CancelTargetLock(FLatentResponse& Response)
{
	Output = ETargetLockOutputPins::OnCancelled;
	Response.FinishAndTriggerIf(true, LATENT_RESPONSE_INFO);
}

void FLatentTargetLock::UpdateTargetLock(FLatentResponse& Response)
{
	if (Started)
	{
		Started = false;
		Output = ETargetLockOutputPins::OnStarted;
		Response.TriggerLink(LATENT_RESPONSE_INFO);
	}
	else
	{
		Output = ETargetLockOutputPins::OnUpdated;
		Response.TriggerLink(LATENT_RESPONSE_INFO);
	}
}

void FLatentTargetLock::StopTargetLock()
{
	CameraLockTarget = nullptr;
	CameraComponent = nullptr;
}
