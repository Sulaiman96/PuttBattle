// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PBSurfaceDefinition.generated.h"

class UNiagaraSystem;

/**
 * A coarse behaviour hook so gameplay can branch on "what kind of surface is
 * this" without string-matching tags. Boost is realised by a placed
 * PBBoostVolume, Hazard by PBHazardVolume — this is for the sampler/UI to read.
 */
UENUM(BlueprintType)
enum class EPBSurfaceHook : uint8
{
	None,
	Boost,
	Sticky,
	Hazard
};

/**
 * Data-driven per-surface ball behaviour (CONVENTIONS §6: adding a surface is a
 * UPBPhysicalMaterial + a UPBSurfaceDefinition + a tag, zero code). The sampler
 * reads RollDragMultiplier to apply a controllable custom rolling-drag force;
 * FrictionOverride / RestitutionOverride are mirrored onto the owning
 * UPBPhysicalMaterial so Chaos uses them for contacts and bounces.
 */
UCLASS(BlueprintType)
class PUTTBATTLE_API UPBSurfaceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Identity key (Surface.Fairway / Surface.Ice / …). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PB|Surface")
	FGameplayTag SurfaceTag;

	/**
	 * Scales the custom rolling-drag force vs the Fairway baseline (1.0).
	 * Ice ≈ 0.25 (slidey), Sand ≈ 3.5, Sticky ≈ 6 (stops fast). The sampler
	 * applies a force opposing horizontal velocity ∝ speed × this.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PB|Surface", meta = (ClampMin = "0.0"))
	float RollDragMultiplier = 1.f;

	/** Chaos surface friction, mirrored to the paired UPBPhysicalMaterial. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PB|Surface", meta = (ClampMin = "0.0"))
	float FrictionOverride = 0.7f;

	/** Chaos bounciness [0,1], mirrored to the paired UPBPhysicalMaterial. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PB|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RestitutionOverride = 0.3f;

	/** Coarse behaviour classification (see EPBSurfaceHook). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PB|Surface")
	EPBSurfaceHook GameplayHook = EPBSurfaceHook::None;

	/** Optional looping roll VFX while a ball moves on this surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PB|Surface")
	TObjectPtr<UNiagaraSystem> RollFX;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("PBSurface")), GetFName());
	}
};
