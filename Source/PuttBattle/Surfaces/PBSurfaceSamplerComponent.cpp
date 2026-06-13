// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBSurfaceSamplerComponent.h"

#include "Ball/PBBallPawn.h"
#include "Components/SphereComponent.h"
#include "PBCollisionChannels.h"
#include "PBPhysicalMaterial.h"
#include "PBSurfaceDefinition.h"
#include "PBSurfaceSubsystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace PBSurfaceCVars
{
	static TAutoConsoleVariable<float> CVarDragCoefficient(
		TEXT("pb.Surface.DragCoefficient"), -1.f,
		TEXT("Override the global rolling-drag coefficient (>=0). -1 = authored."),
		ECVF_Default);
}

UPBSurfaceSamplerComponent::UPBSurfaceSamplerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPBSurfaceSamplerComponent::BeginPlay()
{
	Super::BeginPlay();
}

APBBallPawn* UPBSurfaceSamplerComponent::GetBall() const
{
	return Cast<APBBallPawn>(GetOwner());
}

float UPBSurfaceSamplerComponent::ComputeRollDeceleration(float Speed, float RollDragMultiplier, float Coefficient)
{
	return FMath::Max(Speed, 0.f) * FMath::Max(RollDragMultiplier, 0.f) * FMath::Max(Coefficient, 0.f);
}

FGameplayTag UPBSurfaceSamplerComponent::GetActiveSurfaceTag() const
{
	return ActiveDefinition ? ActiveDefinition->SurfaceTag : FGameplayTag();
}

UPBSurfaceDefinition* UPBSurfaceSamplerComponent::SampleContactDefinition() const
{
	const APBBallPawn* Ball = GetBall();
	if (!Ball || !Ball->CollisionSphere || !GetWorld())
	{
		return nullptr;
	}

	const float Radius = Ball->CollisionSphere->GetScaledSphereRadius();
	const FVector Start = Ball->GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, Radius + GroundTraceSlack);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PBSurfaceSample), /*bTraceComplex=*/false, Ball);
	Params.bReturnPhysicalMaterial = true;

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(PBCollision::Floor);

	FHitResult Hit;
	if (GetWorld()->SweepSingleByObjectType(
			Hit, Start, End, FQuat::Identity, ObjParams,
			FCollisionShape::MakeSphere(Radius * 0.95f), Params))
	{
		if (const UPBPhysicalMaterial* PhysMat = Cast<UPBPhysicalMaterial>(Hit.PhysMaterial.Get()))
		{
			return PhysMat->SurfaceDefinition;
		}
	}
	return nullptr;
}

void UPBSurfaceSamplerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APBBallPawn* Ball = GetBall();
	if (!Ball || !Ball->CollisionSphere || !Ball->CollisionSphere->IsSimulatingPhysics())
	{
		return;
	}

	// Resolve the governing surface: a pushed global override (Ice Age) outranks
	// whatever is physically under the ball.
	UPBSurfaceDefinition* ContactDef = SampleContactDefinition();
	UPBSurfaceDefinition* Resolved = ContactDef;
	if (const UPBSurfaceSubsystem* Surfaces = GetWorld()->GetSubsystem<UPBSurfaceSubsystem>())
	{
		Resolved = Surfaces->ResolveActiveDefinition(ContactDef);
	}
	ActiveDefinition = Resolved;

	// A global override applies everywhere; a contact surface only while grounded.
	const bool bGrounded = ContactDef != nullptr;
	const bool bOverride = Resolved != nullptr && Resolved != ContactDef;
	if (!Resolved || (!bGrounded && !bOverride))
	{
		return;
	}

	const FVector Velocity = Ball->CollisionSphere->GetPhysicsLinearVelocity();
	FVector Horizontal = Velocity; Horizontal.Z = 0.f;
	const float Speed = Horizontal.Size();
	if (Speed < MinDragSpeed)
	{
		return;
	}

	const float Coeff = PBSurfaceCVars::CVarDragCoefficient.GetValueOnGameThread() >= 0.f
		? PBSurfaceCVars::CVarDragCoefficient.GetValueOnGameThread()
		: RollDragCoefficient;
	const float Mult = Resolved ? Resolved->RollDragMultiplier : 1.f;
	const float Decel = ComputeRollDeceleration(Speed, Mult, Coeff);

	// Acceleration opposing horizontal motion (bAccelChange = mass-independent feel).
	const FVector Accel = -Horizontal.GetSafeNormal() * Decel;
	Ball->CollisionSphere->AddForce(Accel, NAME_None, /*bAccelChange=*/true);
}
