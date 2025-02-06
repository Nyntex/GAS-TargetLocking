// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetLockUtilities.h"
#include "Kismet/KismetSystemLibrary.h"

//Heading Angle ignores Z, so I made this
float UTargetLockUtilities::GetAngleToDirection(const FVector& Direction_A, const FVector& Direction_B)
{
	float Dot = FVector::DotProduct(Direction_A, Direction_B);
	float Magnitude = sqrt(
		((Direction_A.X * Direction_A.X) + (Direction_A.Y * Direction_A.Y) + (Direction_A.Z * Direction_A.Z)) *
		((Direction_B.X * Direction_B.X) + (Direction_B.Y * Direction_B.Y) + (Direction_B.Z * Direction_B.Z)));

	return acos(Dot / Magnitude) * (180 / PI);
}

bool UTargetLockUtilities::LineOfSightCheckFromCompToActor(const UObject* WorldContext, const USceneComponent* Origin, 
                                                           const AActor* Target, const TArray<AActor*> IgnoreList, const float LoSDistance)
{
	if (!WorldContext || !Origin || !Target) return false;

	return LineOfSightCheck(WorldContext, Origin->GetComponentLocation(), Target->GetActorLocation(),
		Origin->GetRightVector(), Origin->GetUpVector(), Origin->GetForwardVector(), IgnoreList, LoSDistance);
}

bool UTargetLockUtilities::LineOfSightCheckFromActorToActor(const UObject* WorldContext, const AActor* Origin,
	const AActor* Target, const TArray<AActor*> IgnoreList, const float LoSDistance)
{
	if (!WorldContext || !Origin || !Target) return false;

	return LineOfSightCheck(WorldContext, Origin->GetActorLocation(), Target->GetActorLocation(),
		Origin->GetActorRightVector(), Origin->GetActorUpVector(), Origin->GetActorForwardVector(), IgnoreList, LoSDistance);
}

bool UTargetLockUtilities::LineOfSightCheckFromActorToComp(const UObject* WorldContext, const AActor* Origin,
	const USceneComponent* Target, const TArray<AActor*> IgnoreList, const float LoSDistance)
{
	if (!WorldContext || !Origin || !Target) return false;

	return LineOfSightCheck(WorldContext, Origin->GetActorLocation(), Target->GetComponentLocation(),
		Origin->GetActorRightVector(), Origin->GetActorUpVector(), Origin->GetActorForwardVector(), IgnoreList, LoSDistance);
}

bool UTargetLockUtilities::LineOfSightCheckFromCompToComp(const UObject* WorldContext, const USceneComponent* Origin,
	const USceneComponent* Target, const TArray<AActor*> IgnoreList, const float LoSDistance)
{
	if (!WorldContext || !Origin || !Target) return false;

	return LineOfSightCheck(WorldContext, Origin->GetComponentLocation(), Target->GetComponentLocation(),
		Origin->GetRightVector(), Origin->GetUpVector(), Origin->GetForwardVector(), IgnoreList, LoSDistance);
}

bool UTargetLockUtilities::LineOfSightCheck(const UObject* WorldContext, const FVector& OriginLocation,
	const FVector& TargetLocation, const FVector& RightVector, const FVector& UpVector, const FVector& ForwardVector,
	const TArray<AActor*> IgnoreList, const float LoSDistance)
{
	if (!WorldContext) return false;

	const FVector RayCastTargetsEnemy[] {
		TargetLocation,
		RightVector * LoSDistance + TargetLocation,
		RightVector * -LoSDistance + TargetLocation,
		UpVector * LoSDistance + TargetLocation,
		UpVector * -LoSDistance + TargetLocation,
		ForwardVector * LoSDistance + TargetLocation,
		ForwardVector * -LoSDistance + TargetLocation
	};

	const FVector RayCastTargetsOrigin[] {
		OriginLocation,
		RightVector* LoSDistance + OriginLocation,
		RightVector * -LoSDistance + OriginLocation,
		UpVector * LoSDistance + OriginLocation,
		UpVector * -LoSDistance + OriginLocation,
		ForwardVector * LoSDistance + OriginLocation,
		ForwardVector * -LoSDistance + OriginLocation
	};

	int NonHits = 0;
	for (FVector OriginLoc : RayCastTargetsOrigin)
	{
		for (FVector TargetLoc : RayCastTargetsEnemy)
		{
			FHitResult HitResult;
			UKismetSystemLibrary::LineTraceSingle(WorldContext, OriginLoc, TargetLoc, TraceTypeQuery1, false,
				IgnoreList, EDrawDebugTrace::None, HitResult, true);

			if (!HitResult.bBlockingHit)
			{
				NonHits++;
				if (NonHits >= 2)
				{
					return true;
				}
			}
		}
	}

	return false;
}

float UTargetLockUtilities::FindRotationAddition(float RotationTarget, float RotationOrigin)
{
	if (RotationOrigin < 0) RotationOrigin += 360;
	if (RotationTarget < 0) RotationTarget += 360;
	
	if (RotationTarget < RotationOrigin)
	{
		if ((RotationTarget + 360) - RotationOrigin < RotationOrigin - RotationTarget)
		{
			return (RotationTarget + 360) - RotationOrigin; 
		}
		else
		{
			return (RotationOrigin - RotationTarget) * (-1); 
		}
	}
	else
	{
		if ((RotationOrigin + 360) - RotationTarget < RotationTarget - RotationOrigin)
		{
			return ((RotationOrigin + 360) - RotationTarget) * (-1); 
		}
		else
		{
			return (RotationTarget - RotationOrigin);
		}
	}
}