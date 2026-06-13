// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBTeePad.h"

APBTeePad::APBTeePad(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

FTransform APBTeePad::GetSpawnTransform() const
{
	FTransform T = GetActorTransform();
	T.AddToTranslation(FVector(0.f, 0.f, SpawnZOffset));
	T.SetScale3D(FVector::OneVector);
	return T;
}
