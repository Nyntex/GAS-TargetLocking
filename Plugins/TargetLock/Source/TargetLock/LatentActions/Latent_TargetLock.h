// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TargetLockUtilities.h"
#include "LatentActions.h"
#include "Camera/CameraComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Latent_TargetLock.generated.h"


UENUM()
enum class ETargetLockInputPins : uint8
{
	Start,
	Cancel
};

UENUM()
enum class ETargetLockOutputPins : uint8
{
	OnStarted,
	OnUpdated,
	OnCancelled
};

/**
 * 
 */
UCLASS()
class TARGETLOCK_API ULatent_TargetLock : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/**
	 *
	 * @param WorldContext					Variable holding the object calling this function. Hidden
	 * @param LatentInfo					Variable to hold the information required to process a Latent function. Hidden
	 * @param InputPins						Different Types of Input Pins
	 * @param OutputPins					Different Types of OutputPins
	 * @param CameraComponent				The Camera that should get locked onto a target.
	 * @param LockTarget					Optional. If Lock Target is not given it will automatically try to find a target of type UClass_BaseEnemy
	 * @param MaxAngleToTarget				The maximum angle the camera should be allowed to hold to the Lock Target before it gets forced into position
	 * @param AngleToStartLerp				The Angle at which point the camera will start to smoothly change it's rotation towards the given angle to the target
	 * @param RotateSpeed					The Speed at which the Camera will rotate.
	 * @param HardRotateSpeedMultiplier		The additional speed the camera will rotate at when angel is greater than MaxAngleToTarget
	 * @param MaxDistanceToStartTargetLock	The maximimum distance the camera will allow to find a target to lock on to
	 * @param DoLineOfSightCheck			If there should be an initial line of sight check to see if the target is behind a wall
	 * @param ContinuousLineOfSightCheck	If there should be a continuous line of sight check to see if the chosen target stays in sight. Will Cancel the action if target is behind walls
	 *
	 */
	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContext", Latent, LatentInfo="LatentInfo", ExpandEnumAsExecs="InputPins,OutputPins"), Category="Latent | TargetLock")
	static void LatentTargetLock(UObject* WorldContext, FLatentActionInfo LatentInfo, ETargetLockInputPins InputPins, 
		ETargetLockOutputPins& OutputPins, UCameraComponent* CameraComponent, TArray<TSubclassOf<AActor>> LockableClasses, AActor* LockTarget = nullptr, float MaxAngleToTarget = 40,
		float AngleToStartLerp = 15, float RotateSpeed = 4, float HardRotateSpeedMultiplier = 10,
		float MaxDistanceToStartTargetLock = 1500, bool DoLineOfSightCheck = false, bool ContinuousLineOfSightCheck = false);
};
	
class FLatentTargetLock : public FPendingLatentAction
{
public:
	virtual void UpdateOperation(FLatentResponse& Response) override;

	//Lerps the rotation to rotate to locked target
	void LerpTargetLocked(FLatentResponse& Response);

public:
	TObjectPtr<UCameraComponent> CameraComponent;
	TObjectPtr<AActor> CameraLockTarget;

	//Angle in Degrees
	//The angle where the rotation 
	float MaxAngleToTarget = 40;

	//Angle in Degrees
	float AngleToStartLerp = 15;

	//Rotate Speed. 1 means it takes ~1 second to reach the desired rotation. Higher Values = Faster Rotation.
	//Beware that very high values may result in overshooting because this value directly multiplies the value
	//that gets added to the pitch/yaw
	float RotateSpeed = 4;

	//This ignores "RotateSpeed" by default. X < 1 makes it slower, X > 1 makes it faster.
	//Beware that very high values may result in overshooting because this value directly multiplies the value
	//that gets added to the pitch/yaw
	float HardRotateSpeedMultiplier = 10;

	//How far away a unit is allowed to be, to be eligible for target locking to be applied.
	//This is measured in unreal units / cm.
	float MaxDistanceToStartTargetLock = 1500;

	bool DoLineOfSightCheck = false;
	bool ContinuousLineOfSightCheck = false;

	bool Started = true;
	
public: //REQUIRED
	FLatentActionInfo LatentActionInfo;
	ETargetLockOutputPins& Output;
	
	//Find an enemy
	FLatentTargetLock(FLatentActionInfo& LatentInfo, ETargetLockOutputPins& OutputPins, UCameraComponent* Camera, TArray<TSubclassOf<AActor>> LockableClasses,
		AActor* LockTarget, float MaxAngleToTarget= 40, float AngleToStartLerp = 15, float RotateSpeed = 4, float HardRotateSpeedMultiplier = 10,
		float MaxDistanceToStartTargetLock = 1500, bool DoLineOfSightCheck = false, bool ContinuousLineOfSightCheck = false)
			: CameraComponent(Camera), CameraLockTarget(LockTarget), MaxAngleToTarget(MaxAngleToTarget), AngleToStartLerp(AngleToStartLerp),
	RotateSpeed(RotateSpeed), HardRotateSpeedMultiplier(HardRotateSpeedMultiplier), MaxDistanceToStartTargetLock(MaxDistanceToStartTargetLock),
	DoLineOfSightCheck(DoLineOfSightCheck), ContinuousLineOfSightCheck(ContinuousLineOfSightCheck), LatentActionInfo(LatentInfo), Output(OutputPins)
	{
		if (!CameraComponent) return;
		
		Output = ETargetLockOutputPins::OnStarted;
		if (CameraLockTarget == nullptr)
		{
			AActor* OwningActor = CameraComponent->GetOwner();

			if (!OwningActor) return;

			FVector CameraLocation = CameraComponent->GetComponentLocation();
			FVector CameraForward = CameraComponent->GetForwardVector();

			//Setup possible targets
			//Setup possible targets
			TArray<AActor*> PossibleTargets{};
			TArray<AActor*> TempTargets{};
			for (TSubclassOf<AActor> LockClass : LockableClasses)
			{
				UKismetSystemLibrary::SphereOverlapActors(CameraComponent,
					OwningActor->GetActorLocation(),
					MaxDistanceToStartTargetLock,
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
				if (dist >= ClosestTarget && dist > MaxDistanceToStartTargetLock) continue;

				FVector Direction = Actor->GetActorLocation() - CameraLocation;
				float Angle = UTargetLockUtilities::GetAngleToDirection(CameraForward, Direction);
				if (Angle > MaxAngleToTarget) continue;

				if (dist < MaxDistanceToStartTargetLock && dist < ClosestTarget)
				{
					const TArray<AActor*> IgnoreList{ Actor, OwningActor };
					if(!DoLineOfSightCheck)
					{
						ClosestTarget = dist;
						Target = Actor;
					}
					else if (UTargetLockUtilities::LineOfSightCheckFromCompToActor(CameraComponent, CameraComponent, Actor, IgnoreList, 75) ||
						UTargetLockUtilities::LineOfSightCheckFromActorToActor(CameraComponent, OwningActor, Actor, IgnoreList, 75))
					{
						ClosestTarget = dist;
						Target = Actor;
					}
				}
			}
			CameraLockTarget = Target;
		}
		
	}

public:
	void CancelTargetLock(FLatentResponse& Response);
	void UpdateTargetLock(FLatentResponse& Response);

	void StopTargetLock();
};