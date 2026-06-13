// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBSurfaceSubsystem.h"

#include "PBSurfaceDefinition.h"
#include "TimerManager.h"

int32 UPBSurfaceSubsystem::PushGlobalOverride(UPBSurfaceDefinition* Definition, float Duration)
{
	if (!Definition)
	{
		return INDEX_NONE;
	}

	FOverrideEntry Entry;
	Entry.Id = NextOverrideId++;
	Entry.Definition = Definition;
	const int32 Index = Overrides.Add(MoveTemp(Entry));
	const int32 Id = Overrides[Index].Id;

	if (Duration > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate Del = FTimerDelegate::CreateUObject(this, &UPBSurfaceSubsystem::RemoveOverride, Id);
			World->GetTimerManager().SetTimer(Overrides[Index].ExpiryTimer, Del, Duration, false);
		}
	}
	return Id;
}

void UPBSurfaceSubsystem::PopGlobalOverride()
{
	if (Overrides.Num() == 0)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Overrides.Last().ExpiryTimer);
	}
	Overrides.Pop();
}

void UPBSurfaceSubsystem::RemoveOverride(int32 OverrideId)
{
	const int32 Index = Overrides.IndexOfByPredicate(
		[OverrideId](const FOverrideEntry& E) { return E.Id == OverrideId; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Overrides[Index].ExpiryTimer);
	}
	Overrides.RemoveAt(Index);
}

UPBSurfaceDefinition* UPBSurfaceSubsystem::ResolveActiveDefinition(UPBSurfaceDefinition* ContactDefinition) const
{
	if (Overrides.Num() > 0)
	{
		return Overrides.Last().Definition;
	}
	return ContactDefinition;
}
