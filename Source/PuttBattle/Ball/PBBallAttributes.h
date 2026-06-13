// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBBallAttributes.generated.h"

/**
 * The single channel through which every gameplay modifier reaches a ball
 * (CONVENTIONS §3). Powerup effects mutate these fields server-side; the pawn
 * applies them in one place (APBBallPawn::ApplyAttributes). Never add an ad-hoc
 * replicated bool to the pawn for a new effect — extend this struct instead.
 *
 * Designed for replication now even though Phase 1 is single-player: the struct
 * is a plain replicated UPROPERTY on the pawn and the apply path is identical
 * on server and client.
 */
USTRUCT(BlueprintType)
struct FPBBallAttributes
{
	GENERATED_BODY()

	/** Multiplies shot impulse (Glue ↓, Balloon ↓). 1 = unmodified. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	float PowerMultiplier = 1.f;

	/** Uniform ball scale (Balloon). Drives SetWorldScale3D + a mass update. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	float ScaleMultiplier = 1.f;

	/** Jumble: hide the aim indicator + power feedback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	bool bAimIndicatorHidden = false;

	/** Reverse: ball travels along the drag instead of opposite it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	bool bAimInverted = false;

	/** Shield: blocks the next SingleTarget / AllOthers effect (never Environment). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	bool bShielded = false;

	/** Lock: the ball cannot take a shot while this is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	bool bShotLocked = false;

	/** Ghost Ball: next shot passes through PB_Wall (not PB_Floor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	bool bGhostShotArmed = false;

	/** Anchor: ball stops dead at its next surface contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PB|Ball|Attributes")
	bool bAnchorArmed = false;
};
