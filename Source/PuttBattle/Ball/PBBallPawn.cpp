// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBBallPawn.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Match/PBGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Shot/PBShotComponent.h"
#include "Surfaces/PBSurfaceSamplerComponent.h"

namespace PBBallCVars
{
	// A CVar set to >= 0 overrides the BP/UPROPERTY feel value; -1 means "use the
	// authored value" (CONVENTIONS §1 feel-tuning rule, live PIE tuning for UA-9).
	static TAutoConsoleVariable<float> CVarAngularDamping(
		TEXT("pb.Ball.AngularDamping"), -1.f,
		TEXT("Override ball angular damping (>=0). -1 = use the pawn's authored value."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarLinearDamping(
		TEXT("pb.Ball.LinearDamping"), -1.f,
		TEXT("Override ball linear damping (>=0). -1 = use the pawn's authored value."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarMaxImpulse(
		TEXT("pb.Ball.MaxImpulse"), -1.f,
		TEXT("Override full-power shot impulse (>=0). -1 = use the pawn's authored value."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarMaxSpeed(
		TEXT("pb.Ball.MaxSpeed"), -1.f,
		TEXT("Override the ball's max linear speed clamp in cm/s (>=0). -1 = authored."),
		ECVF_Default);

	/** The locally-controlled ball, for console commands that act on "my ball". */
	static APBBallPawn* FindLocalBall(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<APBBallPawn> It(World); It; ++It)
		{
			if (It->IsLocallyControlled())
			{
				return *It;
			}
		}
		// Fall back to any ball so the test commands work before possession.
		for (TActorIterator<APBBallPawn> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	static void DumpBall(const TArray<FString>& Args, UWorld* World)
	{
		APBBallPawn* Ball = FindLocalBall(World);
		if (!Ball)
		{
			UE_LOG(LogTemp, Warning, TEXT("[pb.Ball.Dump] No ball found."));
			return;
		}
		const FPBBallAttributes& A = Ball->Attributes;
		UE_LOG(LogTemp, Display,
			TEXT("[pb.Ball.Dump] speed=%.1f planar=%.1f | LinDamp=%.3f AngDamp=%.3f MaxImpulse=%.1f MaxSpeed=%.0f | Power×%.2f Scale×%.2f invert=%d lock=%d hideAim=%d shield=%d ghost=%d anchor=%d"),
			Ball->GetSpeed(), Ball->GetPlanarSpeed(),
			Ball->LinearDamping, Ball->AngularDamping, Ball->GetEffectiveMaxImpulse(), Ball->MaxSpeed,
			A.PowerMultiplier, A.ScaleMultiplier, A.bAimInverted, A.bShotLocked,
			A.bAimIndicatorHidden, A.bShielded, A.bGhostShotArmed, A.bAnchorArmed);
	}

	static void SetScale(const TArray<FString>& Args, UWorld* World)
	{
		APBBallPawn* Ball = FindLocalBall(World);
		if (!Ball || Args.Num() < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("[pb.Ball.SetScale] usage: pb.Ball.SetScale <float>"));
			return;
		}
		FPBBallAttributes A = Ball->Attributes;
		A.ScaleMultiplier = FCString::Atof(*Args[0]);
		Ball->SetAttributes(A);
		UE_LOG(LogTemp, Display, TEXT("[pb.Ball.SetScale] ScaleMultiplier=%.2f"), A.ScaleMultiplier);
	}

	static void SetFlag(const TArray<FString>& Args, UWorld* World, const TCHAR* Name)
	{
		APBBallPawn* Ball = FindLocalBall(World);
		if (!Ball)
		{
			return;
		}
		const bool bValue = Args.Num() < 1 ? true : (FCString::Atoi(*Args[0]) != 0);
		FPBBallAttributes A = Ball->Attributes;
		if (FCString::Stricmp(Name, TEXT("invert")) == 0)   { A.bAimInverted = bValue; }
		else if (FCString::Stricmp(Name, TEXT("lock")) == 0)    { A.bShotLocked = bValue; }
		else if (FCString::Stricmp(Name, TEXT("hideaim")) == 0) { A.bAimIndicatorHidden = bValue; }
		Ball->SetAttributes(A);
		UE_LOG(LogTemp, Display, TEXT("[pb.Ball.%s] = %d"), Name, bValue);
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdDump(
		TEXT("pb.Ball.Dump"), TEXT("Log the local ball's effective feel values + attributes."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DumpBall));

	static FAutoConsoleCommandWithWorldAndArgs CmdSetScale(
		TEXT("pb.Ball.SetScale"), TEXT("Set the local ball's ScaleMultiplier (mass + visuals update)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetScale));

	static FAutoConsoleCommandWithWorldAndArgs CmdInvert(
		TEXT("pb.Ball.Invert"), TEXT("Toggle bAimInverted on the local ball (Reverse test)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& A, UWorld* W) { SetFlag(A, W, TEXT("invert")); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdLock(
		TEXT("pb.Ball.Lock"), TEXT("Toggle bShotLocked on the local ball (Lock test)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& A, UWorld* W) { SetFlag(A, W, TEXT("lock")); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdHideAim(
		TEXT("pb.Ball.HideAim"), TEXT("Toggle bAimIndicatorHidden on the local ball (Jumble test)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& A, UWorld* W) { SetFlag(A, W, TEXT("hideaim")); }));
}

APBBallPawn::APBBallPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(6.f); // ~6 cm, 1uu = 1cm
	CollisionSphere->SetCollisionProfileName(TEXT("PB_BallProfile"));
	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetUseCCD(true); // fast balls vs thin walls (D3 ramps)
	CollisionSphere->SetLinearDamping(LinearDamping);
	CollisionSphere->SetAngularDamping(AngularDamping);
	CollisionSphere->SetNotifyRigidBodyCollision(true); // hit events for Anchor/surfaces
	CollisionSphere->SetGenerateOverlapEvents(true);    // cup / checkpoint / hazard triggers
	CollisionSphere->BodyInstance.bOverrideMass = true;
	CollisionSphere->BodyInstance.SetMassOverride(BaseMassKg);
	RootComponent = CollisionSphere;

	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	BallMesh->SetupAttachment(CollisionSphere);
	BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BallMesh->SetGenerateOverlapEvents(false);

	ShotComponent = CreateDefaultSubobject<UPBShotComponent>(TEXT("ShotComponent"));
	SurfaceSampler = CreateDefaultSubobject<UPBSurfaceSamplerComponent>(TEXT("SurfaceSampler"));

	AIControllerClass = nullptr;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void APBBallPawn::BeginPlay()
{
	Super::BeginPlay();
	ApplyPhysicsTuning();
	ApplyAttributes();
}

void APBBallPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Re-apply tuning every frame so pb.Ball.* CVar edits take effect live in PIE.
	ApplyPhysicsTuning();

	// Clamp linear speed so a max-power ramp shot stays believable.
	if (CollisionSphere && CollisionSphere->IsSimulatingPhysics())
	{
		const float MaxSpeedEff = PBBallCVars::CVarMaxSpeed.GetValueOnGameThread() >= 0.f
			? PBBallCVars::CVarMaxSpeed.GetValueOnGameThread()
			: MaxSpeed;
		const FVector V = CollisionSphere->GetPhysicsLinearVelocity();
		if (MaxSpeedEff > 0.f && V.SizeSquared() > FMath::Square(MaxSpeedEff))
		{
			CollisionSphere->SetPhysicsLinearVelocity(V.GetClampedToMaxSize(MaxSpeedEff));
		}
	}
}

void APBBallPawn::FellOutOfWorld(const UDamageType& DmgType)
{
	// Don't let the engine destroy the ball — reset it to its checkpoint (D7).
	if (HasAuthority())
	{
		if (APBGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<APBGameMode>() : nullptr)
		{
			GM->RespawnAtCheckpoint(this);
			return;
		}
	}
	Super::FellOutOfWorld(DmgType);
}

void APBBallPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APBBallPawn, Attributes);
}

#if WITH_EDITOR
void APBBallPawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyPhysicsTuning();
	ApplyAttributes();
}
#endif

void APBBallPawn::SetAttributes(const FPBBallAttributes& NewAttributes)
{
	Attributes = NewAttributes;
	ApplyAttributes();
}

void APBBallPawn::ApplyAttributes()
{
	if (!CollisionSphere)
	{
		return;
	}

	const float Scale = FMath::Max(Attributes.ScaleMultiplier, 0.01f);
	SetActorScale3D(FVector(Scale));

	// Density-preserving mass: a bigger ball is heavier (∝ volume). Updating mass
	// here is what makes a console SetScale visibly change physics behaviour.
	const float NewMass = FMath::Max(BaseMassKg * Scale * Scale * Scale, 0.001f);
	CollisionSphere->SetMassOverrideInKg(NAME_None, NewMass, true);
}

void APBBallPawn::ApplyPhysicsTuning()
{
	if (!CollisionSphere)
	{
		return;
	}
	const float AngEff = PBBallCVars::CVarAngularDamping.GetValueOnGameThread() >= 0.f
		? PBBallCVars::CVarAngularDamping.GetValueOnGameThread() : AngularDamping;
	const float LinEff = PBBallCVars::CVarLinearDamping.GetValueOnGameThread() >= 0.f
		? PBBallCVars::CVarLinearDamping.GetValueOnGameThread() : LinearDamping;

	CollisionSphere->SetAngularDamping(AngEff);
	CollisionSphere->SetLinearDamping(LinEff);
}

float APBBallPawn::GetEffectiveMaxImpulse() const
{
	const float Cvar = PBBallCVars::CVarMaxImpulse.GetValueOnGameThread();
	return Cvar >= 0.f ? Cvar : MaxImpulse;
}

float APBBallPawn::GetPlanarSpeed() const
{
	if (!CollisionSphere)
	{
		return 0.f;
	}
	FVector V = CollisionSphere->GetPhysicsLinearVelocity();
	V.Z = 0.f;
	return V.Size();
}

float APBBallPawn::GetSpeed() const
{
	return CollisionSphere ? CollisionSphere->GetPhysicsLinearVelocity().Size() : 0.f;
}

void APBBallPawn::StopMotion()
{
	if (CollisionSphere)
	{
		CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
}

void APBBallPawn::TeleportToAndStop(const FVector& Location, const FRotator& Rotation)
{
	StopMotion();
	SetActorLocationAndRotation(Location, Rotation, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	StopMotion();
}

void APBBallPawn::OnRep_Attributes()
{
	// OnRep only updates presentation (CONVENTIONS §2): apply scale/mass + let
	// dependent components read the flags. No game logic here.
	ApplyAttributes();
}
