// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetLock/GAS/Tasks/GASTask_EndingAbilityTask.h"

void UGASTask_EndingAbilityTask::StopTask_Implementation()
{
	OnTaskEnded.Broadcast();
	EndTask();
}