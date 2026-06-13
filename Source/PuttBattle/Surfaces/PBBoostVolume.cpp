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
	BoostZone->InitBoxExtent(FVector(50.f, 50.f, 20.f));
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
		Ball->CollisionSphere->AddForce(BoostDir * BoostAcceleration, NAME_None, /*bAccelChange=*/true);
	}
}
