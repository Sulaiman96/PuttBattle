// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBCupActor.h"

#include "Ball/PBBallPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

APBCupActor::APBCupActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CatchZone = CreateDefaultSubobject<USphereComponent>(TEXT("CatchZone"));
	CatchZone->InitSphereRadius(12.f);
	CatchZone->SetCollisionProfileName(TEXT("Trigger")); // QueryOnly, overlaps the ball
	CatchZone->SetGenerateOverlapEvents(true);
	RootComponent = CatchZone;

	CupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CupMesh"));
	CupMesh->SetupAttachment(CatchZone);
	CupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // visual only
}

void APBCupActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CatchZone)
	{
		return;
	}

	// Re-check overlapping balls each tick so a ball that enters fast and then
	// settles below the threshold still drops (DF6).
	TArray<AActor*> Overlapping;
	CatchZone->GetOverlappingActors(Overlapping, APBBallPawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		APBBallPawn* Ball = Cast<APBBallPawn>(Actor);
		if (!Ball || IsBallSunk(Ball))
		{
			continue;
		}
		const bool bSlowEnough = Ball->GetSpeed() < SinkSpeedThreshold;
		const bool bFitsCup = Ball->Attributes.ScaleMultiplier < BalloonedMaxScale;
		if (bSlowEnough && bFitsCup)
		{
			AcceptBall(Ball);
		}
	}
}

bool APBCupActor::IsBallSunk(const APBBallPawn* Ball) const
{
	return SunkBalls.Contains(Ball);
}

void APBCupActor::AcceptBall(APBBallPawn* Ball)
{
	if (!Ball)
	{
		return;
	}
	SunkBalls.Add(Ball);

	// Settle the ball into the cup centre and stop it (presentation in Phase 1).
	Ball->TeleportToAndStop(GetActorLocation(), Ball->GetActorRotation());

	OnBallSunk.Broadcast(Ball);
}
