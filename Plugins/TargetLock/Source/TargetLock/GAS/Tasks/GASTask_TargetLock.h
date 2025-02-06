// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GASTask_EndingAbilityTask.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Camera/CameraComponent.h"
#include "GASTask_TargetLock.generated.h"

USTRUCT(BlueprintType)
struct TARGETLOCK_API FStruct_TargetLockData
{
	GENERATED_BODY()

	//Angle in Degrees
	//The angle the rotation should not overstep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Deg"))
	float MaxAngleToTarget = 40;

	//the angle where the rotation will start to go back towards the target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "Deg"))
	float AngleToStartLerp = 15;

	//Rotate Speed. 1 means it takes ~1 second to reach the desired rotation. Higher Values = Faster Rotation.
	//Beware that very high values may result in overshooting because this value directly multiplies the value
	//that gets added to the pitch/yaw 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotateSpeed = 4;

	//This ignores "RotateSpeed" by default. X < 1 makes it slower, X > 1 makes it faster.
	//Beware that very high values may result in overshooting because this value directly multiplies the value
	//that gets added to the pitch/yaw
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HardRotateSpeedMultiplier = 10;

	//How far away a unit is allowed to be eligible for target locking to be applied.
	//This is measured in unreal units / cm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "CM"))
	float MaxDistanceToStartTargetLock = 1500;

	//Should we do a LineOfSight Check when we find our Target for the first time
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool DoLineOfSightCheck = false;

	//Should we continuously check if the target is still in Line of Sight?
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool ContinuousLineOfSightCheck = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TSubclassOf<AActor>> LockableClasses;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AActor> TargetLockVisualizeActorClass;
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Category = "GAS | Tasks")
class TARGETLOCK_API UGASTask_TargetLock : public UGASTask_EndingAbilityTask
{
	GENERATED_BODY()

public:
	UGASTask_TargetLock(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * Used to create a new Target Lock Task. Should only be called from a Gameplay Ability
	 *
	 * @param OwningAbility The Ability that owns this task.
	 * @param TaskInstanceName The name of the task, can be anything.
	 * @param TaskData The data to use for the target lock. This is used for different configurations.
	 * @param OptionalOwningActor Optional: The Actor that owns the ability and therefore this task. Is used for LineOfSight Checks and is only required to be given if the LineOfSight Check actor is different from it's owner
	 * @param OptionalCamera Optional: The Camera that will be locked onto a target. Is not optional if "OptionalOwningActor" is set to different actor than this tasks actual owning actor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "True"))
	static UGASTask_TargetLock* StartTargetLock(
			UGameplayAbility* OwningAbility,
			FName TaskInstanceName,
			FStruct_TargetLockData TaskData,
			AActor* OptionalOwningActor = nullptr,
			UCameraComponent* OptionalCamera= nullptr);
	
protected:
	virtual void TickTask(float DeltaTime) override;

	//Gameplay Task version of "Begin Play"
	virtual void Activate() override;

	virtual void OnDestroy(bool bInOwnerFinished) override;

	//Sets up the data, receives the camera component of the player and searches for a
	//UClass_BaseEnemy to target. These things may get exposed in the future.
	void SetupTargetLock(UCameraComponent* OptionalCam = nullptr, AActor* OptionalOwner = nullptr);

	//Lerps the rotation to rotate to locked target
	void LerpTargetLocked(double DeltaTime);

	//The camera that gets rotated towards the @CameraLockTarget
	UPROPERTY(BlueprintReadOnly, meta=(ExposeOnSpawn="true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	//The configuration for the task
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS | Target Locking Task")
	FStruct_TargetLockData Configuration;

	//The target we want to keep in the center of the camera
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAS | Target Locking Task")
	TObjectPtr<AActor> CameraLockTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability | Target Lock")
	AActor* TargetLockVisualizeActor;
	
	/**
	 * @return True if locking onto a target.
	 */
	UFUNCTION(BlueprintCallable, Category = "GAS | Target Locking Task")
	bool IsLockingOnTarget() const;

	virtual void StopTask_Implementation() override;
};
