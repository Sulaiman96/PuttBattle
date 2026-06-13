// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBCameraRig.h"

#include "Camera/CameraComponent.h"

APBCameraRig::APBCameraRig()
{
	PrimaryActorTick.bCanEverTick = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;

	// Compressed isometric diorama framing by default (tunable per instance).
	Camera->SetRelativeRotation(FRotator(-52.f, 0.f, 0.f));
	Camera->FieldOfView = 35.f;
}
