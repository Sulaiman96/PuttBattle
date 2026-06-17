// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBCupActor.h"

#include "Ball/PBBallPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Match/PBGameMode.h"

APBCupActor::APBCupActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CatchZone = CreateDefaultSubobject<USphereComponent>(TEXT("CatchZone"));
	CatchZone->InitSphereRadius(CatchRadius); // seed; BeginPlay re-applies the authored value
	CatchZone->SetCollisionProfileName(TEXT("Trigger")); // QueryOnly, overlaps the ball
	CatchZone->SetGenerateOverlapEvents(true);
	RootComponent = CatchZone;

	CupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CupMesh"));
	CupMesh->SetupAttachment(CatchZone);
	CupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // visual only
}

void APBCupActor::BeginPlay()
{
	Super::BeginPlay();
	if (CatchZone)
	{
		CatchZone->SetSphereRadius(CatchRadius);
	}
}

void APBCupActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Sinking is gameplay truth (ends the hole / scores) → authority only
	// (CONVENTIONS §2). Standalone is authority, so behaviour is unchanged now; in
	// Phase 3 this stops every client independently sinking the replicated ball.
	if (!CatchZone || !HasAuthority())
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

	// Settle the ball into the cup centre and stop it.
	Ball->TeleportToAndStop(GetActorLocation(), Ball->GetActorRotation());

	// Server records the finish: order, server timestamp, spectate routing, and the
	// all-finished hole-end check (APBMatchGameMode; base mode no-ops offline).
	if (APBGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<APBGameMode>() : nullptr)
	{
		GM->NotifyBallSunk(Ball);
	}

	OnBallSunk.Broadcast(Ball);
}
