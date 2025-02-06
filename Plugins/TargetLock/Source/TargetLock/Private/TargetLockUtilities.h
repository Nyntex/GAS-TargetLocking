// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TargetLockUtilities.generated.h"

/**
 * 
 */
UCLASS()
class TARGETLOCK_API UTargetLockUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Vector")
	static float GetAngleToDirection(const FVector& Direction_A, const FVector& Direction_B);
	
	//Different Setup for LineOfSightCheck with less arguments where a component is the origin and an actor is the target
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = "Line Trace | Line of Sight")
	static bool LineOfSightCheckFromCompToActor(const UObject* WorldContext, const USceneComponent* Origin, const AActor* Target, const TArray<AActor*> IgnoreList, const float LoSDistance);
	//Different Setup for LineOfSightCheck with less arguments where an actor is the origin and an actor is the target
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = "Line Trace | Line of Sight")
	static bool LineOfSightCheckFromActorToActor(const UObject* WorldContext, const AActor* Origin, const AActor* Target, const TArray<AActor*> IgnoreList, const float LoSDistance);
	//Different Setup for LineOfSightCheck with less arguments where an actor is the origin and a component is the target
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = "Line Trace | Line of Sight")
	static bool LineOfSightCheckFromActorToComp(const UObject* WorldContext, const AActor* Origin, const USceneComponent* Target, const TArray<AActor*> IgnoreList, const float LoSDistance);
	//Different Setup for LineOfSightCheck with less arguments where a component is the origin and a component is the target
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = "Line Trace | Line of Sight")
	static bool LineOfSightCheckFromCompToComp(const UObject* WorldContext, const USceneComponent* Origin, const USceneComponent* Target, const TArray<AActor*> IgnoreList, const float LoSDistance);

	//Line of Sight check for all sorts of things
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = "Line Trace | Line of Sight")
	static bool LineOfSightCheck(const UObject* WorldContext, const FVector& OriginLocation, const FVector& TargetLocation, const FVector& RightVector, const FVector& UpVector, const FVector& ForwardVector, const TArray<AActor*> IgnoreList, const float LoSDistance);

	//Finds the amount of rotation to add to reach the desired rotation by checking which way is the shortest.
	UFUNCTION(BlueprintPure, Category="Rotation")
	static float FindRotationAddition(float RotationTarget, float RotationOrigin);
};
