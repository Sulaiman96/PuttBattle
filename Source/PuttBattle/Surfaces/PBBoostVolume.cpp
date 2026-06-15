// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBBoostVolume.h"

#include "Ball/PBBallPawn.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h" // Ball->CollisionSphere->AddForce needs the full type

APBBoostVolume::APBBoostVolume()
{
	PrimaryActorTick.bCanEverTick = true;

	BoostZone = CreateDefaultSubobject<UBoxComponent>(TEXT("BoostZone"));
	BoostZone->InitBoxExtent(ZoneExtent); // seed; BeginPlay re-applies the authored value
	BoostZone->SetCollisionProfileName(TEXT("Trigger"));
	BoostZone->SetGenerateOverlapEvents(true);
	RootComponent = BoostZone;

	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(BoostZone);
	DirectionArrow->ArrowColor = FColor::Cyan;
	DirectionArrow->ArrowSize = 1.5f;
}

void APBBoostVolume::BeginPlay()
{
	Super::BeginPlay();
	BoostZone->SetBoxExtent(ZoneExtent);
	BoostZone->OnComponentBeginOverlap.AddDynamic(this, &APBBoostVolume::OnBeginOverlap);
	BoostZone->OnComponentEndOverlap.AddDynamic(this, &APBBoostVolume::OnEndOverlap);
}

void APBBoostVolume::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	if (APBBallPawn* Ball = Cast<APBBallPawn>(Other))
	{
		OverlappingBalls.Add(Ball);
	}
}

void APBBoostVolume::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (APBBallPawn* Ball = Cast<APBBallPawn>(Other))
	{
		OverlappingBalls.Remove(Ball);
	}
}

void APBBoostVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (OverlappingBalls.Num() == 0)
	{
		return;
	}

	const FVector BoostDir = GetActorForwardVector().GetSafeNormal();
	for (auto It = OverlappingBalls.CreateIterator(); It; ++It)
	{
		APBBallPawn* Ball = It->Get();
		if (!Ball || !Ball->CollisionSphere)
		{
			It.RemoveCurrent();
			continue;
		}

		// Altering a ball's trajectory is a physics outcome — authority only in
		// Phase 3 (CONVENTIONS §2). No-op in standalone (authority is local); the
		// guard keeps a client from fighting the replicated server position later.
		if (!Ball->HasAuthority())
		{
			continue;
		}

		// Self-bounding: stop pushing once the ball is already at the cap along the
		// boost axis, so the pad doesn't accumulate speed without limit.
		if (MaxBoostSpeed > 0.f)
		{
			const float AlongAxis = FVector::DotProduct(Ball->CollisionSphere->GetPhysicsLinearVelocity(), BoostDir);
			if (AlongAxis >= MaxBoostSpeed)
			{
				continue;
			}
		}

		Ball->CollisionSphere->AddForce(BoostDir * BoostAcceleration, NAME_None, /*bAccelChange=*/true);
	}
}
