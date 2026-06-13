// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PBSurfaceSubsystem.generated.h"

class UPBSurfaceDefinition;

/**
 * Whole-map surface override stack (CONVENTIONS / plans/02 T2.1). The Ice Age
 * powerup (Phase 6) pushes a global Ice override for 10 s and pops it — and that
 * must need zero changes here, which is the whole point of building the stack
 * now. Samplers consult ResolveActiveDefinition() so the top override wins over
 * the surface actually under the ball.
 */
UCLASS()
class PUTTBATTLE_API UPBSurfaceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Push a global override; Duration > 0 auto-pops, <= 0 stays until popped. Returns its id. */
	UFUNCTION(BlueprintCallable, Category = "PB|Surface")
	int32 PushGlobalOverride(UPBSurfaceDefinition* Definition, float Duration);

	/** Pop the most recently pushed override (if any). */
	UFUNCTION(BlueprintCallable, Category = "PB|Surface")
	void PopGlobalOverride();

	/** Remove a specific override by the id returned from PushGlobalOverride. */
	UFUNCTION(BlueprintCallable, Category = "PB|Surface")
	void RemoveOverride(int32 OverrideId);

	/** True if any global override is currently active. */
	UFUNCTION(BlueprintPure, Category = "PB|Surface")
	bool HasActiveOverride() const { return Overrides.Num() > 0; }

	/**
	 * The definition that should govern behaviour: the top of the override stack
	 * if non-empty, otherwise the definition under the ball (may be null).
	 */
	UPBSurfaceDefinition* ResolveActiveDefinition(UPBSurfaceDefinition* ContactDefinition) const;

private:
	struct FOverrideEntry
	{
		int32 Id = 0;
		TObjectPtr<UPBSurfaceDefinition> Definition = nullptr;
		FTimerHandle ExpiryTimer;
	};

	TArray<FOverrideEntry> Overrides;
	int32 NextOverrideId = 1;
};
