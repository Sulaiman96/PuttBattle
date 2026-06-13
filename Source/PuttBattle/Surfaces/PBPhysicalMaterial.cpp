// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBPhysicalMaterial.h"

#include "PBSurfaceDefinition.h"

UPBPhysicalMaterial::UPBPhysicalMaterial()
{
	// Sensible defaults for a rolling ball; per-surface values come from the def.
	Friction = 0.7f;
	Restitution = 0.3f;
}

void UPBPhysicalMaterial::PostLoad()
{
	Super::PostLoad();
	SyncFromDefinition();
}

#if WITH_EDITOR
void UPBPhysicalMaterial::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SyncFromDefinition();
}
#endif

void UPBPhysicalMaterial::SyncFromDefinition()
{
	if (SurfaceDefinition)
	{
		Friction = SurfaceDefinition->FrictionOverride;
		Restitution = SurfaceDefinition->RestitutionOverride;
	}
}
