// Fill out your copyright notice in the Description page of Project Settings.


#include "Latent_TargetLock.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#define DEBUG_ENABLED false
#define LATENT_RESPONSE_INFO LatentActionInfo.ExecutionFunction, LatentActionInfo.Linkage, LatentActionInfo.CallbackTarget

void ULatent_TargetLock::LatentTargetLock(UObject* WorldContext, FLatentActionInfo LatentInfo,
                                          ETargetLockInputPins InputPins, ETargetLockOutputPins& OutputPins, UCameraComponent* CameraComponent, TArray<TSubclassOf<AActor>> LockableClasses,
                                          AActor* LockTarget, float MaxAngleToTarget, float AngleToStartLerp, float RotateSpeed, float HardRotateSpeedMultiplier,
                                          float MaxDistanceToStartTargetLock, bool DoLineOfSightCheck, bool ContinuousLineOfSightCheck)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);

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
	float DeltaTime = std::min(Response.ElapsedTime(), 0.1f);

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
	
#if WITH_EDITOR && DEBUG_ENABLED
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, "Angle: " + FString::SanitizeFloat(Angle));

		DrawDebugLine(CameraComponent->GetWorld(), CameraLocation + (FVector::DownVector * 100), CameraLocation + (CameraDirection)+(FVector::DownVector * 100), FColor::Blue, false, 0, 0, 10);
		DrawDebugLine(CameraComponent->GetWorld(), CameraLocation + (FVector::DownVector * 100), TargetLocation, FColor::Red, false, 0, 0, 5);
		DrawDebugLine(CameraComponent->GetWorld(), CameraLocation + (FVector::DownVector * 100) + ProjectedVector, CameraLocation + ProjectedVector + (NormalizedAngledDirection * DistFromProjectedToTarget), FColor::Green, false, 0, 0, 10);

		DrawDebugSphere(CameraComponent->GetWorld(), CameraLocation + (FVector::DownVector * 100) + ProjectedVector, 10, 16, FColor::Magenta, false, 0, 0, 10);
	}
#endif

	if (Angle + 1.f >= MaxAngleToTarget)
	{

		const float TargetYaw = TargetSoftRotator.Yaw;
		float ControllerYaw = Controller->GetControlRotation().Yaw;
		ControllerYaw -= ControllerYaw - TargetYaw >= 180 ? 360 : 0; //this ensures that it doesn't matter what the rotations are
		float CalcYaw = TargetHardRotator.Yaw - ControllerYaw;
		CalcYaw *= DeltaTime * HardRotateSpeedMultiplier;


		const float TargetPitch = TargetSoftRotator.Pitch;
		float ControllerPitch = Controller->GetControlRotation().Pitch;
		ControllerPitch -= ControllerPitch - TargetPitch >= 180 ? 360 : 0;
		float CalcPitch = TargetHardRotator.Pitch - ControllerPitch;
		CalcPitch *= -DeltaTime * HardRotateSpeedMultiplier;

		//Forcefully rotate Camera
		//Controller->AddYawInput(CalcYaw);
		//Controller->AddPitchInput(CalcPitch);

		Controller->SetControlRotation(FRotator(-CalcPitch, CalcYaw, 0) + Controller->GetControlRotation());

		//debug prints and stuff
#if WITH_EDITOR && DEBUG_ENABLED
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Hard Rotate Camera");
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, FString::SanitizeFloat(Angle));
		}
#endif
	}

	if (Angle >= AngleToStartLerp) //This -0.5 is to be able to rotate the camera
	{

		//Make smooth yaw input
		const float TargetYaw = TargetSoftRotator.Yaw;
		float ControllerYaw = Controller->GetControlRotation().Yaw;
		ControllerYaw -= ControllerYaw - TargetYaw >= 180 ? 360 : 0;
		//ControllerYaw += 180;
		float CalcYaw = (TargetYaw)-ControllerYaw;
		CalcYaw *= RotateSpeed * DeltaTime;


		//make smooth pitch input
		const float TargetPitch = TargetSoftRotator.Pitch;
		float ControllerPitch = Controller->GetControlRotation().Pitch;
		ControllerPitch -= ControllerPitch - TargetPitch >= 180 ? 360 : 0;
		float CalcPitch = TargetSoftRotator.Pitch - ControllerPitch;
		CalcPitch *= -RotateSpeed * DeltaTime;

		Controller->SetControlRotation(FRotator(-CalcPitch, CalcYaw, 0) + Controller->GetControlRotation());

		//debug prints
#if WITH_EDITOR && DEBUG_ENABLED
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Black, "------------");
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Controller Yaw: " + FString::SanitizeFloat(Controller->GetControlRotation().Yaw));
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Cyan, "Target Yaw: " + FString::SanitizeFloat(TargetSoftRotator.Yaw));
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow, "Adding Yaw: " + FString::SanitizeFloat(CalcYaw));
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow, " ");
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Controller Pitch: " + FString::SanitizeFloat(Controller->GetControlRotation().Pitch));
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Cyan, "Target Pitch: " + FString::SanitizeFloat(TargetSoftRotator.Pitch));
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Yellow, "Adding Pitch: " + FString::SanitizeFloat(CalcPitch));
			GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Black, "------------");
		}
#endif
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
