// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBCameraRig.h"

#include "Camera/CameraComponent.h"

APBCameraRig::APBCameraRig()
{
	// Tick is needed only for the auto-orbit overview; the Tick body early-returns
	// when bAutoOrbit is false, so a plain fixed rig costs nothing.
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;

	// Compressed isometric diorama framing by default (tunable per instance).
	Camera->SetRelativeRotation(FRotator(-52.f, 0.f, 0.f));
	Camera->FieldOfView = 35.f;
}

void APBCameraRig::BeginPlay()
{
	Super::BeginPlay();
	// Snapshot the placed location before Tick starts relocating the camera root.
	OrbitPivot = GetActorLocation();
}

void APBCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bAutoOrbit || !Camera)
	{
		return; // fixed rig — nothing to animate
	}

	// Drive the sweep from world time, not an accumulator, so all clients land on
	// (near) the same angle without replicating anything, and the camera keeps
	// moving whether or not the local player is currently looking through it.
	const UWorld* World = GetWorld();
	const float Time = World ? World->GetTimeSeconds() : 0.f;
	const float Rad = FMath::DegreesToRadians(Time * OrbitDegreesPerSecond);

	const FVector Offset(FMath::Cos(Rad) * OrbitRadius, FMath::Sin(Rad) * OrbitRadius, OrbitHeight);
	const FVector CamLoc = OrbitPivot + Offset;
	const FRotator Look = (OrbitPivot - CamLoc).Rotation(); // aim back at the pivot
	Camera->SetWorldLocationAndRotation(CamLoc, Look);
	Camera->SetFieldOfView(OrbitFieldOfView);
}
