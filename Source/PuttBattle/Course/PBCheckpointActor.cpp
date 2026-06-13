// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBCheckpointActor.h"

#include "Ball/PBBallPawn.h"
#include "Components/SphereComponent.h"
#include "Match/PBGameMode.h"

APBCheckpointActor::APBCheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ActivationZone = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationZone"));
	ActivationZone->InitSphereRadius(60.f);
	ActivationZone->SetCollisionProfileName(TEXT("Trigger"));
	ActivationZone->SetGenerateOverlapEvents(true);
	RootComponent = ActivationZone;
}

void APBCheckpointActor::BeginPlay()
{
	Super::BeginPlay();
	ActivationZone->OnComponentBeginOverlap.AddDynamic(this, &APBCheckpointActor::OnBeginOverlap);
}

FTransform APBCheckpointActor::GetRespawnTransform() const
{
	FTransform T = GetActorTransform();
	T.AddToTranslation(FVector(0.f, 0.f, RespawnZOffset));
	T.SetScale3D(FVector::OneVector);
	return T;
}

void APBCheckpointActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	APBBallPawn* Ball = Cast<APBBallPawn>(Other);
	if (!Ball || ActivatedBalls.Contains(Ball))
	{
		return;
	}
	ActivatedBalls.Add(Ball);

	// Authority records the highest checkpoint reached (server-tracked, D7).
	if (Ball->HasAuthority())
	{
		if (APBGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<APBGameMode>() : nullptr)
		{
			GM->ActivateCheckpoint(Ball, this);
		}
	}

	// Local-only activation FX (P1). Replicated per-player filtering arrives in P3.
	if (Ball->IsLocallyControlled())
	{
		OnActivatedForLocalPlayer();
	}
}
