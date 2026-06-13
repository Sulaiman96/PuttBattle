// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBShotTypes.generated.h"

/**
 * The fully-resolved intent of a shot: a flat aim direction and a normalised
 * power. Built client-side from drag input, then executed. In Phase 3 this
 * becomes the payload of the Server_ExecuteShot RPC unchanged (CONVENTIONS §2),
 * so it carries everything the server needs to reproduce the shot and nothing
 * client-only (no screen coords, no camera).
 */
USTRUCT(BlueprintType)
struct FPBShotRequest
{
	GENERATED_BODY()

	/** Flat, normalised aim direction in world XY (Z is always 0 — D3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot")
	FVector2D Dir = FVector2D::ZeroVector;

	/** Drag power through the ease curve, clamped [0,1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Shot")
	float Power01 = 0.f;
};

/** Drag-aim-release state machine (CONVENTIONS / plans/01 T1.2). */
UENUM(BlueprintType)
enum class EPBShotState : uint8
{
	/** Ball not yet at rest, or no aim in progress. */
	Idle,
	/** Player is dragging; preview is shown. */
	Aiming,
	/** Shot fired; waiting for the ball to come to rest. */
	Rolling
};
