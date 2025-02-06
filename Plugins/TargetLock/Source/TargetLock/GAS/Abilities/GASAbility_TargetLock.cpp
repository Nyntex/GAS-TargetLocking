// Fill out your copyright notice in the Description page of Project Settings.

#include "GASAbility_TargetLock.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UGASAbility_TargetLock::UGASAbility_TargetLock(const class FObjectInitializer& Initializer)
	: Super(Initializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::Type::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::Type::ReplicateNo;
	bRetriggerInstancedAbility = true;
}

bool UGASAbility_TargetLock::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
                                                const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	bool Previous = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	
	return true;
}

void UGASAbility_TargetLock::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	SCOPED_NAMED_EVENT_FSTRING(FString("Activate Target Lock"), FColor::Green);
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	//End Target Lock if we are already locking onto a something
	if (IsLockingOnTarget)
	{
		(FString("Stopping Target Lock"), FColor::Green);
		IsLockingOnTarget = false;
		if (TargetLockTask)
		{
			TargetLockTask->StopTask();
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	//Start a new Target Lock
	TargetLockTask = UGASTask_TargetLock::StartTargetLock(
		this,
		"TargetLockTask",
		TargetLockData,
		GetOwningActorFromActorInfo()
		);
	
	if (!TargetLockTask) //End Ability if something went wrong
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}
	TargetLockTask->OnTaskEnded.AddDynamic(this, &UGASAbility_TargetLock::OnTaskEnded);
	TargetLockTask->ReadyForActivation();
	
	IsLockingOnTarget = true;
}

void UGASAbility_TargetLock::OnTaskEnded()
{
	SCOPED_NAMED_EVENT_FSTRING(FString("TargetLock Ended"), FColor::Green);
	IsLockingOnTarget = false;
	CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false);
}
