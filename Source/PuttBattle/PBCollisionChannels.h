// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Named mirrors of the object channels defined in DefaultEngine.ini
 * (CONVENTIONS §4). The .ini is the source of truth for the responses; these
 * constants just let C++ trace against them without bare ECC_GameTraceChannelN
 * magic numbers. If the channel order in DefaultEngine.ini ever changes, update
 * here too.
 *
 *   PB_Ball  = ECC_GameTraceChannel1
 *   PB_Floor = ECC_GameTraceChannel2
 *   PB_Wall  = ECC_GameTraceChannel3
 */
namespace PBCollision
{
	static constexpr ECollisionChannel Ball  = ECC_GameTraceChannel1;
	static constexpr ECollisionChannel Floor = ECC_GameTraceChannel2;
	static constexpr ECollisionChannel Wall  = ECC_GameTraceChannel3;
}
