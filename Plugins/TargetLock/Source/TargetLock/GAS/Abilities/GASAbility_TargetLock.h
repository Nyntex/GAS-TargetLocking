// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TargetLock/GAS/Tasks/GASTask_TargetLock.h"
#include "GASAbility_TargetLock.generated.h"

/**
 * This ability lets the given camera be rotated smoothly towards a target that gets chosen by the ability. 
 * This ability is nothing more than a runner for the task. However, we wanted to encapsulate it in an Ability to use the task like any other ability.
 */
UCLASS(Abstract)
class TARGETLOCK_API UGASAbility_TargetLock : public UGameplayAbility
{
	GENERATED_BODY()

	UGASAbility_TargetLock(const class FObjectInitializer& Initializer);
	
protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnTaskEnded();
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Ability | Target Lock")
	TObjectPtr<UGASTask_TargetLock> TargetLockTask;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability | Target Lock")
	FStruct_TargetLockData TargetLockData;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ability | State")
	bool IsLockingOnTarget{false};
	
};
