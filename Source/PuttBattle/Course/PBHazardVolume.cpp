// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBHazardVolume.h"

#include "Ball/PBBallPawn.h"
#include "Components/BoxComponent.h"
#include "Match/PBGameMode.h"

APBHazardVolume::APBHazardVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	HazardZone = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardZone"));
	HazardZone->InitBoxExtent(FVector(100.f, 100.f, 50.f));
	HazardZone->SetCollisionProfileName(TEXT("Trigger"));
	HazardZone->SetGenerateOverlapEvents(true);
	RootComponent = HazardZone;
}

void APBHazardVolume::BeginPlay()
{
	Super::BeginPlay();
	HazardZone->OnComponentBeginOverlap.AddDynamic(this, &APBHazardVolume::OnBeginOverlap);
}

void APBHazardVolume::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	APBBallPawn* Ball = Cast<APBBallPawn>(Other);
	if (!Ball)
	{
		return;
	}

	const FVector EntryLocation = Ball->GetActorLocation();

	// Respawn is gameplay truth → authority only (CONVENTIONS §2).
	if (Ball->HasAuthority())
	{
		if (APBGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<APBGameMode>() : nullptr)
		{
			GM->RespawnAtCheckpoint(Ball);
		}
	}

	OnBallEntered(EntryLocation);
}
