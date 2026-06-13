// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Surfaces/PBSurfaceSamplerComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Verifies, deterministically and without a physics scene, that the surface
 * roll-drag model produces measurably different roll distances and the right
 * ordering: Ice rolls furthest, then Fairway, Sand, Sticky (plans/02 T2.1
 * "Done when"). It integrates the same UPBSurfaceSamplerComponent::
 * ComputeRollDeceleration used at runtime, so a feel change to the model is
 * caught here too. Distances are logged for tuning reference.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPBSurfaceRollDistanceTest,
	"PuttBattle.Tests.Surfaces.RollDistanceOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/** Integrate a ball decelerating under the roll-drag model to a stop (cm). */
	float SimulateRollDistance(float RollDragMultiplier)
	{
		constexpr float Coefficient = 1.5f;   // matches the sampler default
		constexpr float InitialSpeed = 2000.f;
		constexpr float Dt = 1.f / 120.f;
		constexpr float RestSpeed = 5.f;       // matches the at-rest threshold
		constexpr int32 MaxSteps = 100000;

		float Speed = InitialSpeed;
		float Distance = 0.f;
		for (int32 Step = 0; Step < MaxSteps && Speed > RestSpeed; ++Step)
		{
			const float Decel = UPBSurfaceSamplerComponent::ComputeRollDeceleration(Speed, RollDragMultiplier, Coefficient);
			Speed = FMath::Max(Speed - Decel * Dt, 0.f);
			Distance += Speed * Dt;
		}
		return Distance;
	}
}

bool FPBSurfaceRollDistanceTest::RunTest(const FString& Parameters)
{
	const float IceDist     = SimulateRollDistance(0.25f); // Surface.Ice
	const float FairwayDist = SimulateRollDistance(1.0f);  // Surface.Fairway (baseline)
	const float SandDist    = SimulateRollDistance(3.5f);  // Surface.Sand
	const float StickyDist  = SimulateRollDistance(6.0f);  // Surface.Sticky

	UE_LOG(LogTemp, Display,
		TEXT("[RollDistance] Ice=%.1f Fairway=%.1f Sand=%.1f Sticky=%.1f (cm)"),
		IceDist, FairwayDist, SandDist, StickyDist);

	TestTrue(TEXT("All surfaces produce a positive roll distance"),
		IceDist > 0.f && FairwayDist > 0.f && SandDist > 0.f && StickyDist > 0.f);
	TestTrue(TEXT("Ice rolls further than Fairway"), IceDist > FairwayDist);
	TestTrue(TEXT("Fairway rolls further than Sand"), FairwayDist > SandDist);
	TestTrue(TEXT("Sand rolls further than Sticky"), SandDist > StickyDist);

	// Distinctness: Ice should be dramatically longer than Sticky, not a sliver.
	TestTrue(TEXT("Ice roll is at least 4x the Sticky roll"), IceDist > StickyDist * 4.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
