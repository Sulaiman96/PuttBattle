// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Shot/PBShotComponent.h"
#include "Shot/PBShotTypes.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

/**
 * The server-side cheat guard for an incoming shot request (plans/03 T3.1
 * "add a cheat-attempt automation test"). Exercises the same pure predicate
 * Server_RequestShot_Validate uses, so the exploit-rejection contract — honest
 * payloads pass, forged/garbage payloads are rejected (which disconnects the
 * sender) — is regression-checked without a network session. State gates
 * (phase / at-rest / lock) are runtime concerns and are covered in PIE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPBShotValidationTest,
	"PuttBattle.Tests.Shot.RequestValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPBShotValidationTest::RunTest(const FString& Parameters)
{
	auto MakeRequest = [](float X, float Y, float Power)
	{
		FPBShotRequest R;
		R.Dir = FVector2D(X, Y);
		R.Power01 = Power;
		return R;
	};

	// Honest shots across the legal power range pass.
	TestTrue(TEXT("Zero-power request is structurally valid"),
		UPBShotComponent::IsShotRequestStructurallyValid(MakeRequest(1.f, 0.f, 0.f)));
	TestTrue(TEXT("Mid-power request is structurally valid"),
		UPBShotComponent::IsShotRequestStructurallyValid(MakeRequest(0.f, -1.f, 0.5f)));
	TestTrue(TEXT("Full-power request is structurally valid"),
		UPBShotComponent::IsShotRequestStructurallyValid(MakeRequest(-0.7f, 0.7f, 1.f)));

	// Cheat / corruption: over-powered shot is rejected (would over-drive the impulse).
	TestFalse(TEXT("Power above 1 is rejected"),
		UPBShotComponent::IsShotRequestStructurallyValid(MakeRequest(1.f, 0.f, 5.f)));
	TestFalse(TEXT("Negative power is rejected"),
		UPBShotComponent::IsShotRequestStructurallyValid(MakeRequest(1.f, 0.f, -1.f)));

	// Non-finite payloads (NaN / Inf) are rejected before they reach the physics body.
	const float NaNf = FMath::Sqrt(-1.f);
	const float Inf = std::numeric_limits<float>::infinity();
	TestFalse(TEXT("NaN power is rejected"),
		UPBShotComponent::IsShotRequestStructurallyValid(MakeRequest(1.f, 0.f, NaNf)));
	TestFalse(TEXT("Infinite direction is rejected"),
		UPBShotComponent::IsShotRequestStructurallyValid(MakeRequest(Inf, 0.f, 0.5f)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
