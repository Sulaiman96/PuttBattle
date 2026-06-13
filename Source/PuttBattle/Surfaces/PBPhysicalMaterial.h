// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PBPhysicalMaterial.generated.h"

class UPBSurfaceDefinition;

/**
 * The bridge from a rendered surface to its gameplay definition. Assigned to a
 * floor mesh's material; the sampler trace lands on it and resolves the active
 * UPBSurfaceDefinition. On load/edit it copies the definition's friction +
 * restitution onto its own inherited UPhysicalMaterial fields so Chaos uses them
 * for contacts — the definition stays the single source of truth (CONVENTIONS §6).
 */
UCLASS()
class PUTTBATTLE_API UPBPhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	UPBPhysicalMaterial();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PB|Surface")
	TObjectPtr<UPBSurfaceDefinition> SurfaceDefinition;

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/** Pull Friction/Restitution from the definition into the engine fields. */
	void SyncFromDefinition();
};
