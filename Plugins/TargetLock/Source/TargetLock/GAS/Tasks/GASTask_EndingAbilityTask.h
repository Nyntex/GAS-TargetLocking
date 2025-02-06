// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GASTask_EndingAbilityTask.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskEndedSignature);
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, Category = "GAS | Tasks")
class TARGETLOCK_API UGASTask_EndingAbilityTask : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnTaskEndedSignature OnTaskEnded;

	/**
	 *  When overriding this in child classes make sure to call parent implementation at the end of yours or
	 *  add a broadcast and call EndTask(), since this is effectively what happens here.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GAS | Tasks")
	void StopTask();
};
