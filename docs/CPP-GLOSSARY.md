# CPP-GLOSSARY.md — Putt Battle C++ source map

> **What this is.** A file-by-file map of the `PuttBattle` C++ module: what each
> file/class does, and — front and centre — **which members are reachable from
> Blueprint**. Read this when you need to find where something lives or what you
> can call/bind/implement from a BP subclass or widget graph.
>
> **What this is *not*.** It is not an Unreal-concepts tutorial (that's
> [docs/LEARNING.md](LEARNING.md)), not the design authority (that's
> [docs/README.md](README.md)), and not the rulebook (that's
> [CLAUDE.md](../CLAUDE.md) / [docs/CONVENTIONS.md](CONVENTIONS.md)). It is a
> derived index — **the code is the source of truth**; if this doc and the code
> disagree, the code wins and this doc is stale (fix it — see
> [§ Rules for future agents](#rules-for-future-agents)).
>
> Last regenerated against `Source/PuttBattle` after the Phase 3 (multiplayer
> core) merge. Engine: UE 5.8. Module: `PuttBattle` (single runtime module).

---

## How to read the Blueprint columns

Four different macros mean four different relationships between C++ and Blueprint.
Don't conflate them — for a novice this is the most common source of confusion:

| Tag in this doc | UE macro | Direction | What it means for you in BP |
|---|---|---|---|
| **Callable** | `UFUNCTION(BlueprintCallable)` | BP → C++ | You can **call** it from a BP graph. Has side effects. |
| **Pure** | `UFUNCTION(BlueprintPure)` | BP → C++ | You can **read** it from a BP graph (no exec pin; a getter). |
| **Event (implement)** | `UFUNCTION(BlueprintImplementableEvent)` | C++ → BP | C++ **calls** it; you **implement** the body in the BP subclass/widget graph (usually visuals/SFX). Empty if you don't. |
| **Event (override)** | `UFUNCTION(BlueprintNativeEvent)` | C++ → BP | Has a C++ default; BP **may override**. (None in the codebase yet.) |
| **Delegate (bind)** | `UPROPERTY(BlueprintAssignable)` on a `DECLARE_DYNAMIC_MULTICAST_DELEGATE` | C++ → BP | A multicast event you **bind** to in BP (`Bind Event to …`). |
| **Property RW/RO** | `UPROPERTY(BlueprintReadWrite / BlueprintReadOnly)` | both | A variable BP can read (RO) or read+write (RW). `EditAnywhere` ones also show in the Details panel — these are the **feel-tuning surface** ([CONVENTIONS §1](CONVENTIONS.md)). |

**Authority reminder** ([CONVENTIONS §2](CONVENTIONS.md)): just because something is
`BlueprintCallable` does not mean it's safe to call on a client. Mutators that
change game truth (shots, attributes, scoring, respawn) are **server-only** and
guard on `HasAuthority()` — calling them on a client silently does nothing.

---

## Folder map (one subsystem per folder)

| Folder | Subsystem | Key classes |
|---|---|---|
| [`Ball/`](#ball) | The ball pawn + the attribute channel | `APBBallPawn`, `FPBBallAttributes` |
| [`Shot/`](#shot) | Drag-aim-release shot loop | `UPBShotComponent`, `FPBShotRequest`, `EPBShotState` |
| [`Surfaces/`](#surfaces) | Per-surface roll physics + global overrides | `UPBSurfaceSamplerComponent`, `UPBSurfaceDefinition`, `UPBPhysicalMaterial`, `UPBSurfaceSubsystem`, `APBBoostVolume` |
| [`Course/`](#course) | Hole geometry: tee, checkpoints, cup, hazards | `APBTeePad`, `APBCheckpointActor`, `APBCupActor`, `APBHazardVolume` |
| [`Camera/`](#camera) | Per-hole rigs + client camera director | `APBCameraRig`, `UPBCameraSubsystem` |
| [`Match/`](#match-modes) | GameModes, GameStates, PlayerState, PlayerController | `APBGameMode`, `APBMatchGameMode`, `APBLobbyGameMode`, `APBMatchGameState`, `APBLobbyGameState`, `APBPlayerState`, `APBPlayerController`, match types |
| [`Net/`](#net) | Steam session lifecycle (host/find/join) | `UPBSessionSubsystem` |
| [`UI/`](#ui) | HUD widget data plumbing | `UPBHUDWidget` |
| [`Powerups/` `Progression/` `Spectate/`](#placeholders) | Folder-reserving stubs (no logic yet) | `UPB*Placeholder` |
| [`Tests/`](#tests) | Automation Spec tests for pure logic | `FPBShotValidationTest`, `FPBSurfaceRollDistanceTest` |
| [module root](#module-root) | Module boilerplate + collision channel aliases | `PBCollisionChannels.h`, `PuttBattle.cpp` |

There are also four cross-cutting reference tables at the end:
[Console variables](#appendix-a--console-variable-pb-master-list),
[Collision channels](#appendix-b--collision-channels),
[Key data-flow pipelines](#appendix-c--key-pipelines-how-the-pieces-talk),
and the [maintenance rules](#rules-for-future-agents).

---

## Ball

> The player **is** the ball (D1): a physics-simulating sphere pawn with one
> central attribute struct as the only channel for gameplay modifiers. Server
> owns the physics; a `BP_BallPawn` subclass wires the cosmetic mesh + power
> curve and carries the tuned feel values so iteration never needs a recompile.

### `FPBBallAttributes` — [`Ball/PBBallAttributes.h`](../Source/PuttBattle/Ball/PBBallAttributes.h)

`USTRUCT` (BlueprintType). **The single channel through which every gameplay
modifier reaches a ball** ([CONVENTIONS §3](CONVENTIONS.md)). Powerup effects mutate
these fields *server-side*; the pawn applies them in one place
(`APBBallPawn::ApplyAttributes`). Never add an ad-hoc replicated bool to the pawn
for a new effect — extend this struct.

**Properties (all `EditAnywhere, BlueprintReadWrite`):**

| Field | Powerup | Effect |
|---|---|---|
| `float PowerMultiplier` (=1) | Glue, Balloon | Multiplies shot impulse. |
| `float ScaleMultiplier` (=1) | Balloon | Uniform ball scale → `SetWorldScale3D` + mass recalc. |
| `bool bAimIndicatorHidden` | Jumble | Hides aim indicator + power feedback. |
| `bool bAimInverted` | Reverse | Ball travels *along* the drag, not opposite. |
| `bool bShielded` | Shield | Blocks the next SingleTarget/AllOthers effect. |
| `bool bShotLocked` | Lock | Ball cannot take a shot. |
| `bool bGhostShotArmed` | Ghost Ball | Next shot passes through `PB_Wall` (not `PB_Floor`). |
| `bool bAnchorArmed` | Anchor | Ball stops dead at next surface contact. |

### `APBBallPawn` — [`Ball/PBBallPawn.h`](../Source/PuttBattle/Ball/PBBallPawn.h) / [`.cpp`](../Source/PuttBattle/Ball/PBBallPawn.cpp)

`UCLASS : APawn`. Owns the physics sphere (Chaos), the shot component, and the
surface sampler. Every feel constant is reachable three ways: the UPROPERTY, the
`BP_BallPawn` default, and a `pb.Ball.*` CVar (live PIE override; a CVar `≥ 0`
overrides the property).

**Blueprint functions:**

| Signature | Kind | What it does |
|---|---|---|
| `void ApplyAttributes()` | Callable | Re-applies `Attributes` to the pawn (scale → mass, etc.). The one apply path. |
| `float GetPlanarSpeed() const` | Pure | Horizontal (XY) speed cm/s, ignoring vertical. |
| `float GetSpeed() const` | Pure | True 3D speed cm/s. |
| `void StopMotion()` | Callable | Zero linear + angular velocity (respawn / anchor / timeout). |
| `void TeleportToAndStop(FVector Location, FRotator Rotation)` | Callable | Teleport + stop (respawn path). |

> `SetAttributes()` and `GetShotComponent()` exist in C++ but are **not**
> BP-exposed; `SetAttributes` is the server-side mutation entry point that calls
> `ApplyAttributes` after writing.

**Key Blueprint properties** (RO = component refs; RW = feel/net tuning):

- **Components (`VisibleAnywhere, BlueprintReadOnly`):** `CollisionSphere`
  (root, simulates, profile `PB_BallProfile`, CCD on), `BallMesh` (cosmetic, no
  collision), `ShotComponent`, `SurfaceSampler`.
- **Feel (`EditAnywhere, BlueprintReadWrite`, category `PB|Ball|Feel`):**
  `BaseRadiusCm` (=6), `BaseMassKg` (=0.045, mass ∝ scale³), `LinearDamping`
  (=0.05), `AngularDamping` (=0.6), `MaxSpeed` (=3000, planar cap), `MaxImpulse`
  (=90, full-power impulse before `PowerMultiplier`).
- **Net (`PB|Ball|Net`):** `MovingNetUpdateHz` (=30), `RestNetUpdateHz` (=5),
  `NetRestSpeedThreshold` (=5) — adaptive replication-rate throttle.
- **Attributes:** `FPBBallAttributes Attributes` (`ReplicatedUsing =
  OnRep_Attributes`, also `EditAnywhere, BlueprintReadWrite`).

**Network / notes:** Server-authoritative physics; `PredictiveInterpolation` set
in `BeginPlay` smooths simulated proxies (remote balls);
`bReplicatePhysicsToAutonomousProxy` corrects the owning client. Replication is
throttled — fast while moving, slow at rest (a net-rate throttle, *not* dormancy,
because a dormant body can freeze mid-motion under boost/gust). `FellOutOfWorld`
routes to checkpoint respawn (D7), not destruction. `OnRep_Attributes` is
presentation-only. See `pb.Ball.*` in [Appendix A](#appendix-a--console-variable-pb-master-list).

---

## Shot

> The drag-aim-release loop: capture screen drag on the client → build an
> RPC-safe `FPBShotRequest` → validate against cheats → execute a flat impulse
> **server-side** with stroke accounting. State machine `Idle → Aiming →
> Rolling → Idle`. All feel constants are `EditAnywhere` + shadowed by CVars.

### `UPBShotComponent` — [`Shot/PBShotComponent.h`](../Source/PuttBattle/Shot/PBShotComponent.h) / [`.cpp`](../Source/PuttBattle/Shot/PBShotComponent.cpp)

`UActorComponent` living on `APBBallPawn`. Replicated (`SetIsReplicatedByDefault`)
so the owning client can route its `Server_RequestShot` RPC.

**Blueprint functions:**

| Signature | Kind | What it does |
|---|---|---|
| `bool CanAim() const` | Pure | True if a new shot may begin (idle, not locked, phase allows, not finished). |
| `EPBShotState GetShotState() const` | Pure | Current state (Idle/Aiming/Rolling). |
| `void ResetForNewHole()` | Callable | Reset local shot state for a hole restart. |
| `void OnUpdateAimPreview(FVector Start, FVector End, float Power01)` | Event (implement) | **BP draws the aim preview** each frame (Niagara/spline). |
| `void OnHideAimPreview()` | Event (implement) | **BP hides the preview** on aim end/cancel. |
| `void OnLocalShotFired(float Power01)` | Event (implement) | **BP plays swing SFX/VFX** the instant of release (before server round-trip). |

> **Input entry points** `StartAim` / `UpdateAim` / `ReleaseAim` / `CancelAim`
> are plain C++ (called by `APBPlayerController` only), **not** BP-exposed.

**Delegates to bind (`BlueprintAssignable`):** `OnAimUpdated`
(`FPBAimUpdated`, fired each frame while aiming — power + indicator-hidden) and
`OnAimEnded` (`FPBAimEnded`, on release/cancel). The HUD binds these.

**Feel properties (`EditAnywhere, BlueprintReadWrite`):** `MaxDragScreenFraction`
(=0.28), `DeadZonePixels` (=24), `PowerCurve` (`UCurveFloat`, drag→power easing),
`AtRestSpeedThreshold` (=5), `AtRestDuration` (=0.5), `RollTimeout` (=12),
`ShotConfirmTimeout` (=1.5), `PreviewLengthAtFullPower` (=600), `PreviewMinLength`
(=50).

**RPC:** `Server_RequestShot` — Server, Reliable, **WithValidation**.
`_Validate` rejects only structurally-impossible payloads (NaN/Inf, power out of
range); state gates (phase, at-rest, lock, finished) are re-checked silently in
`_Implementation` so honest latency never disconnects a client. The actual
impulse (`ExecuteShot`) is server-only, flat (D3), and `bVelChange=false` so ball
mass (Balloon) affects distance. `IsShotRequestStructurallyValid` is a static
helper shared with the automation test.

### `FPBShotRequest` / `EPBShotState` — [`Shot/PBShotTypes.h`](../Source/PuttBattle/Shot/PBShotTypes.h)

- `FPBShotRequest` (USTRUCT, BlueprintType): the RPC payload — `FVector2D Dir`
  (flat world-XY, Z=0) + `float Power01` (eased, clamped [0,1]). Both
  `EditAnywhere, BlueprintReadWrite`. Carries **no** screen/camera data.
- `EPBShotState` (UENUM, BlueprintType): `Idle`, `Aiming`, `Rolling`.

---

## Surfaces

> Data-driven surface behaviour: each surface type is a `UPBSurfaceDefinition`
> (roll-drag + Chaos friction/restitution + hook) paired with a
> `UPBPhysicalMaterial`. The sampler on the ball reads the floor under it and
> applies a custom roll-drag force; a world subsystem can push global overrides
> (e.g. Ice Age). **Adding a surface = 1 definition + 1 physical material + 1
> tag, zero code** ([CONVENTIONS §6](CONVENTIONS.md)).

### `UPBSurfaceDefinition` — [`Surfaces/PBSurfaceDefinition.h`](../Source/PuttBattle/Surfaces/PBSurfaceDefinition.h)

`UPrimaryDataAsset` (primary asset type `PBSurface`). The per-surface data asset.
No BP functions. Properties (`EditAnywhere, BlueprintReadOnly`): `SurfaceTag`
(`Surface.*`), `RollDragMultiplier` (vs Fairway 1.0 — Ice ~0.25, Sand ~3.5,
Sticky ~6), `FrictionOverride`, `RestitutionOverride`, `GameplayHook`
(`EPBSurfaceHook`: None/Boost/Sticky/Hazard), `RollFX` (`UNiagaraSystem`,
optional looping roll VFX). `EPBSurfaceHook` is a UENUM.

### `UPBPhysicalMaterial` — [`Surfaces/PBPhysicalMaterial.h`](../Source/PuttBattle/Surfaces/PBPhysicalMaterial.h) / [`.cpp`](../Source/PuttBattle/Surfaces/PBPhysicalMaterial.cpp)

`UPhysicalMaterial`. The bridge from a floor mesh's material to its gameplay
definition. Holds `SurfaceDefinition` (`EditAnywhere, BlueprintReadOnly`) and
mirrors the definition's friction/restitution into the Chaos fields via
`SyncFromDefinition` on `PostLoad` / `PostEditChangeProperty` (definition is the
single source of truth). No BP functions.

> **Authoring gotcha** ([LEARNING.md](LEARNING.md), "Physical material from a
> *simple*-collision trace"): the `UPBPhysicalMaterial` must be the component's
> phys-material **override** — assigning it only to the render material resolves
> nothing on the sampler's simple trace.

### `UPBSurfaceSamplerComponent` — [`Surfaces/PBSurfaceSamplerComponent.h`](../Source/PuttBattle/Surfaces/PBSurfaceSamplerComponent.h) / [`.cpp`](../Source/PuttBattle/Surfaces/PBSurfaceSamplerComponent.cpp)

`UActorComponent` (BlueprintSpawnableComponent) on the ball. Down-traces (object
type `PB_Floor`) for the surface, consults the override stack, applies roll-drag
**only while grounded** and above `MinDragSpeed`.

**Blueprint functions:** `UPBSurfaceDefinition* GetActiveDefinition() const`
(Pure) and `FGameplayTag GetActiveSurfaceTag() const` (Pure).

**Feel properties (`EditAnywhere, BlueprintReadWrite`):** `FallbackDefinition`,
`RollDragCoefficient` (mirrored by `pb.Surface.DragCoefficient`),
`GroundTraceSlack`, `MinDragSpeed`. Drag is applied as acceleration
(`bAccelChange=true`, mass-independent). `ComputeRollDeceleration` is a static
method shared with the roll-distance automation test.

### `UPBSurfaceSubsystem` — [`Surfaces/PBSurfaceSubsystem.h`](../Source/PuttBattle/Surfaces/PBSurfaceSubsystem.h) / [`.cpp`](../Source/PuttBattle/Surfaces/PBSurfaceSubsystem.cpp)

`UWorldSubsystem`. A LIFO stack of global surface overrides (last-in wins) for
map-wide effects.

| Signature | Kind | What it does |
|---|---|---|
| `int32 PushGlobalOverride(UPBSurfaceDefinition* Definition, float Duration)` | Callable | Push override; `Duration>0` auto-pops, `≤0` until popped. Returns an ID. |
| `void PopGlobalOverride()` | Callable | Pop the most recent override. |
| `void RemoveOverride(int32 OverrideId)` | Callable | Remove a specific override by ID. |
| `bool HasActiveOverride() const` | Pure | True if any override is active. |

`FPBSurfaceOverride` (USTRUCT) wraps the definition in a `UPROPERTY` to keep it
GC-reachable. `ResolveActiveDefinition` (non-BP) returns the top override else the
contact definition.

### `APBBoostVolume` — [`Surfaces/PBBoostVolume.h`](../Source/PuttBattle/Surfaces/PBBoostVolume.h) / [`.cpp`](../Source/PuttBattle/Surfaces/PBBoostVolume.cpp)

`AActor`. A placed boost pad that accelerates overlapping balls along its arrow,
self-bounded by a speed cap. No BP functions. Properties (`EditAnywhere,
BlueprintReadWrite`): `BoostAcceleration` (cm/s²), `MaxBoostSpeed` (cm/s along
axis; ≤0 disables cap), `ZoneExtent`. Components (`VisibleAnywhere,
BlueprintReadOnly`): `BoostZone` (box root), `DirectionArrow`. **Authority-only:**
force is applied in `Tick` only when `HasAuthority()`.

---

## Course

> Per-hole geometry actors placed in the level: spawn (tee), progress markers
> (checkpoints), the goal (cup), and reset zones (hazards). Each pairs
> **server-authoritative game logic** with a **locally-filtered cosmetic
> Event** for the affected player.

### `APBTeePad` — [`Course/PBTeePad.h`](../Source/PuttBattle/Course/PBTeePad.h) / [`.cpp`](../Source/PuttBattle/Course/PBTeePad.cpp)

`APlayerStart` subclass (so the GameMode's default spawn logic picks it up). No
ticking. `FTransform GetSpawnTransform() const` (**Pure**) is the single source
of truth for spawn/respawn, lifted by `SpawnZOffset` (`EditAnywhere`, =10) so the
ball settles.

### `APBCheckpointActor` — [`Course/PBCheckpointActor.h`](../Source/PuttBattle/Course/PBCheckpointActor.h) / [`.cpp`](../Source/PuttBattle/Course/PBCheckpointActor.cpp)

`AActor`. Ordered respawn anchor. On entry, authority tells the GameMode to record
the checkpoint; the activating player gets local FX.

| Member | Kind | What it does |
|---|---|---|
| `FTransform GetRespawnTransform() const` | Pure | Where the ball reappears (offset by `RespawnZOffset`). |
| `void OnActivatedForLocalPlayer()` | Event (implement) | **BP plays activation FX** (local player only). |

Properties: `int32 CheckpointIndex` (`EditAnywhere`, RW — highest activated wins),
`float RespawnZOffset` (RW), `USphereComponent* ActivationZone` (RO, 60 cm trigger).

### `APBCupActor` — [`Course/PBCupActor.h`](../Source/PuttBattle/Course/PBCupActor.h) / [`.cpp`](../Source/PuttBattle/Course/PBCupActor.cpp)

`AActor`. The hole. Re-evaluates overlapping balls every tick: a ball sinks only
if below `SinkSpeedThreshold` **and** not Ballooned (`ScaleMultiplier <
BalloonedMaxScale`) **and** within `CatchRadius`. Sinking is **authority-only**.

**Delegate to bind:** `FPBBallSunk OnBallSunk` (`BlueprintAssignable`,
`DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam` — payload is the ball). HUD toast +
scoring hook here.

Properties (`EditAnywhere, BlueprintReadWrite`): `SinkSpeedThreshold` (=120),
`BalloonedMaxScale` (=1.3), `CatchRadius` (=12 — the third difficulty dial).
Components (RO): `CatchZone` (sphere trigger), `CupMesh`. `IsBallSunk` /
`AcceptBall` are private (server-only).

### `APBHazardVolume` — [`Course/PBHazardVolume.h`](../Source/PuttBattle/Course/PBHazardVolume.h) / [`.cpp`](../Source/PuttBattle/Course/PBHazardVolume.cpp)

`AActor`. Water/void zone. On overlap, authority routes the ball through the same
`GameMode::RespawnAtCheckpoint` as a KillZ fall (zero velocity, no penalty).
`void OnBallEntered(FVector EntryLocation)` (**Event (implement)** — BP plays the
splash). Properties: `FGameplayTag HazardTag` (`Hazard.Water`/`Hazard.Void` —
cosmetic only), `UBoxComponent* HazardZone` (RO).

---

## Camera

> Camera is **client-only presentation, never replicated** ([CONVENTIONS §2](CONVENTIONS.md)).
> Per-hole rigs (`APBCameraRig`) provide fixed or auto-orbiting framings; a
> world subsystem (`UPBCameraSubsystem`) cycles the view target. (The drag-aim
> follow cameras themselves live on `APBPlayerController` — see [Match](#match-state--player).)
> Aim stays correct after a camera change because the shot component re-derives
> camera-relative aim every frame from the live rotation.

### `APBCameraRig` — [`Camera/PBCameraRig.h`](../Source/PuttBattle/Camera/PBCameraRig.h) / [`.cpp`](../Source/PuttBattle/Camera/PBCameraRig.cpp)

`AActor` with a `UCameraComponent` root. Two modes: static isometric framing
(default ~52° pitch, FOV 35), or `bAutoOrbit` — a slow time-driven 360° sweep
(world-time based, so all clients see the same sweep without replication). No BP
functions. Properties (`EditAnywhere, BlueprintReadWrite`): `Priority` (cycle sort
order), `bAutoOrbit`, and the orbit set (`OrbitDegreesPerSecond` =6,
`OrbitRadius` =2200, `OrbitHeight` =1600, `OrbitFieldOfView` =55, each
`EditCondition bAutoOrbit`). `Camera` component is RO.

### `UPBCameraSubsystem` — [`Camera/PBCameraSubsystem.h`](../Source/PuttBattle/Camera/PBCameraSubsystem.h) / [`.cpp`](../Source/PuttBattle/Camera/PBCameraSubsystem.cpp)

`UWorldSubsystem`, client-only. Collects rigs, sorts by `Priority`, blends the
PC's view target.

| Signature | Kind | What it does |
|---|---|---|
| `void CycleCamera(APlayerController* PC)` | Callable | Advance to the next rig (wrapping), blend with `BlendTime`. |
| `void ActivateFirstRig(APlayerController* PC)` | Callable | Snap to the lowest-priority rig (no blend). |
| `void RefreshRigs()` | Callable | Re-scan the level for rigs and re-sort. |

`float BlendTime` (`EditAnywhere`, RW, =0.3). Lazy-collects rigs on first use.

---

## Match (Modes)

> The referees. `APBGameMode` is the base (offline feel-testing + shared
> checkpoint/respawn logic); `APBMatchGameMode` adds the multiplayer phase
> machine; `APBLobbyGameMode` runs the pre-match lobby and seamless-travels the
> party into the match. **GameModes are server-only** — none of their methods are
> BP-exposed except the two base respawn helpers.

### `APBGameMode` — [`Match/PBGameMode.h`](../Source/PuttBattle/Match/PBGameMode.h) / [`.cpp`](../Source/PuttBattle/Match/PBGameMode.cpp)

`AGameModeBase`. Owns checkpoint tracking (stored on PlayerState, D7), respawn,
and hole restart. Sets `DefaultPawnClass`/`PlayerControllerClass`/`PlayerStateClass`
so even offline play gets a PB PlayerState.

| Signature | Kind | What it does |
|---|---|---|
| `void RestartHole(APlayerController* PC)` | Callable | Ball → tee, reset per-hole PlayerState + shot component. |
| `void RespawnAtCheckpoint(APBBallPawn* Ball)` | Callable | Ball → highest checkpoint (or tee), zero velocity, no stroke. The one respawn path for water/void/KillZ (D7). |

`NotifyBallSunk` and `ActivateCheckpoint` are virtual/internal (not BP-exposed);
`APBMatchGameMode` overrides `NotifyBallSunk` to record finish order.

### `APBMatchGameMode` — [`Match/PBMatchGameMode.h`](../Source/PuttBattle/Match/PBMatchGameMode.h) / [`.cpp`](../Source/PuttBattle/Match/PBMatchGameMode.cpp)

`APBGameMode` subclass. Owns **every** phase transition (writes `EPBMatchPhase`
onto `APBMatchGameState`) and stamps server-authoritative finish order/time at
sink. Loop: HoleIntro → Countdown → Playing → HoleResults → next hole /
MatchResults. `bUseSeamlessTravel` on. No BP functions.

Tunable durations (`EditAnywhere, BlueprintReadWrite`): `WarmupSeconds` (=2),
`HoleIntroSeconds` (=4, D20), `CountdownSeconds` (=3), `HoleResultsSeconds` (=8).
Drives the machine with a single `FTimerHandle`. Reads `FPBMatchSettings` from the
`UPBSessionSubsystem` at match start. (Phase-4 timer rules / ShotGrace are stubs
here.)

### `APBLobbyGameMode` — [`Match/PBLobbyGameMode.h`](../Source/PuttBattle/Match/PBLobbyGameMode.h) / [`.cpp`](../Source/PuttBattle/Match/PBLobbyGameMode.cpp)

`AGameModeBase`. Seats 2–6, identifies the host (first to `PostLogin`), validates
+ clamps host settings (D11/DF1), and once everyone is ready seamless-travels to
the match map carrying settings via the session subsystem. No ball in the lobby
(`DefaultPawnClass` null). No BP functions. Properties (`EditDefaultsOnly,
BlueprintReadWrite`): `MatchMapURL`, `MatchGameModeURL` (forces the match referee
via `?game=`). `HostSetMatchSettings` / `RequestStartMatch` are internal (called
by the controller's RPCs, not BP).

### Match types — [`Match/PBMatchTypes.h`](../Source/PuttBattle/Match/PBMatchTypes.h)

- `EPBMatchPhase` (UENUM, BlueprintType): `Lobby, HoleIntro, Countdown, Playing,
  ShotGrace, HoleResults, MatchResults`. Replicated; cheaper than tags for a
  fixed small state. `PBPhaseAllowsShots()` is an inline helper (only `Playing`).
- `FPBMatchSettings` (USTRUCT, BlueprintType): host rules, all `EditAnywhere,
  BlueprintReadWrite` with clamps — `HoleCount` (3–9), `MapTimeSeconds`
  (60–300, =90), `FirstFinishClampSeconds` (10–60, =20),
  `TArray<FPBPowerupWeight> PowerupWeights`.
- `FPBPowerupWeight` (USTRUCT, BlueprintType): `FGameplayTag PowerupTag` +
  `float Weight` (relative, normalised at draw time, DF5).

---

## Match (State & Player)

> Replicated facts (server writes, clients read) + per-player input. GameStates
> hold the match/lobby truth; PlayerState holds per-player truth; PlayerController
> drives one client's input and cameras.

### `APBMatchGameState` — [`Match/PBMatchGameState.h`](../Source/PuttBattle/Match/PBMatchGameState.h) / [`.cpp`](../Source/PuttBattle/Match/PBMatchGameState.cpp)

`AGameStateBase` (deliberately *Base* — avoids Epic's `MatchState` FSM fighting
`EPBMatchPhase`). Replicates phase, hole index, phase deadline (server-time), and
active settings. **All reads, no writes from BP.**

| Signature | Kind | What it does |
|---|---|---|
| `EPBMatchPhase GetMatchPhase() const` | Pure | Current phase (gate input/UI). |
| `int32 GetCurrentHoleIndex() const` | Pure | Hole being played. |
| `const FPBMatchSettings& GetMatchSettings() const` | Pure | Active host rules. |
| `float GetPhaseTimeRemaining() const` | Pure | Drift-free countdown via replicated server clock. |
| `bool ArePlayerShotsAllowed() const` | Pure | True only in Playing/ShotGrace. |
| `bool AreAllPlayersFinished() const` | Pure | All connected players sunk → drives HoleResults. |

**Delegate:** `FPBMatchPhaseChanged OnMatchPhaseChanged` (`BlueprintAssignable`)
— UI binds to refresh on phase change. Setters are server-only (`HasAuthority`).

### `APBLobbyGameState` — [`Match/PBLobbyGameState.h`](../Source/PuttBattle/Match/PBLobbyGameState.h) / [`.cpp`](../Source/PuttBattle/Match/PBLobbyGameState.cpp)

`AGameStateBase`. Replicates the host's `FPBMatchSettings` and a ready tally.

| Signature | Kind | What it does |
|---|---|---|
| `const FPBMatchSettings& GetMatchSettings() const` | Pure | Host-authored settings (all clients preview). |
| `bool AreAllPlayersReady() const` | Pure | ≥2 players and all `IsReady()` (D16). |
| `int32 GetNumPlayers() const` | Pure | Seat count (`PlayerArray.Num()`). |

### `APBPlayerState` — [`Match/PBPlayerState.h`](../Source/PuttBattle/Match/PBPlayerState.h) / [`.cpp`](../Source/PuttBattle/Match/PBPlayerState.cpp)

`APlayerState`. The single truth for strokes, checkpoint, and finish placement.
`CopyProperties` carries fields across seamless travel.

| Signature | Kind | What it does |
|---|---|---|
| `int32 GetStrokes() const` | Pure | Strokes this hole. |
| `int32 GetTotalScore() const` | Pure | Running match score (placeholder until Phase 4). |
| `int32 GetCheckpointIndex() const` | Pure | Highest activated checkpoint (`INDEX_NONE` = tee). |
| `bool IsFinished() const` | Pure | Has sunk this hole. |
| `int32 GetFinishOrder() const` | Pure | Finish placement (0 = first). |
| `double GetFinishServerTime() const` | Pure | Server-time of sink (authoritative ordering key). |
| `bool IsReady() const` | Pure | Lobby ready flag. |

**Delegate:** `FPBStrokesChanged OnStrokesChanged` (`BlueprintAssignable`) — HUD
binds. Mutators (`AddStroke`, `ResetForNewHole`, `SetCheckpointIndex` (never
regresses), `MarkFinished` (idempotent), `SetReady`) are server-only.
`OnFinishedChanged` is a `BlueprintImplementableEvent` for BP presentation.

### `APBPlayerController` — [`Match/PBPlayerController.h`](../Source/PuttBattle/Match/PBPlayerController.h) / [`.cpp`](../Source/PuttBattle/Match/PBPlayerController.cpp)

`APlayerController`. Captures LMB drag → feeds the ball's shot component; cycles 3
camera modes (2 follow framings + overview rig); RMB orbit; carries lobby input
to the GameMode via Server RPCs. Input is client-only.

| Signature | Kind | What it does |
|---|---|---|
| `bool IsSpectating() const` | Pure | In placeholder spectate (post-sink wait; Phase 8 expands). |
| `void Server_SetReady(bool bReady)` | Callable + Server RPC | Toggle lobby ready (→ PlayerState). |
| `void Server_SetMatchSettings(FPBMatchSettings NewSettings)` | Callable + Server RPC | Host pushes settings (GameMode re-checks host + clamps). |
| `void Server_RequestStartMatch()` | Callable + Server RPC | Host requests start (GameMode re-checks host + all-ready). |

> The three `Server_*` are Reliable **WithValidation** — validation only filters
> garbage (finite checks); real authority gates live in `APBLobbyGameMode`.
> `Client_EnterSpectate`/`Client_ExitSpectate` are Client RPCs (not BP).

`FPBFollowCamPreset` (USTRUCT, `EditAnywhere, BlueprintReadWrite`): a framing
(spring-arm pitch, length, FOV, auto-face-hole). **Feel properties:**
`FollowCamPresets` (`EditAnywhere`, RW — index 0 = play cam, 1 = shot cam),
`OrbitYawSpeed` (RW, deg/px). **Input assets** (`EditDefaultsOnly,
BlueprintReadOnly`, wired on a BP subclass): `PuttMappingContext`,
`MappingContextPriority`, `DragShotAction`, `CycleCameraAction`, `CancelAction`,
`Camera1Action`, `Camera2Action`, `Camera3Action`, `OrbitCameraAction`. See
`pb.Camera.*` in [Appendix A](#appendix-a--console-variable-pb-master-list).

---

## Net

### `UPBSessionSubsystem` — [`Net/PBSessionSubsystem.h`](../Source/PuttBattle/Net/PBSessionSubsystem.h) / [`.cpp`](../Source/PuttBattle/Net/PBSessionSubsystem.cpp)

`UGameInstanceSubsystem` — **survives map travel** (unlike a world subsystem), so
it carries the host's match settings from lobby into match. Wraps the Online
Subsystem (Steam) session interface. All operations are async + fire delegates;
the front-end BP UI drives them.

| Signature | Kind | What it does |
|---|---|---|
| `void CreateRoom(int32 MaxPlayers, FString RoomName)` | Callable | Host: advertise a Steam lobby (MaxPlayers clamped 2–6), travel the listen server to the lobby map. |
| `void FindRooms(int32 MaxResults = 50)` | Callable | Query Steam lobbies; results land in `SearchSettings`, fires `OnFindRoomsComplete`. |
| `void JoinRoomByIndex(int32 SearchResultIndex)` | Callable | Join a found room, then `ClientTravel` on success. |
| `void DestroyRoom()` | Callable | Tear down the current session (host or client). |
| `void SetPendingMatchSettings(const FPBMatchSettings& InSettings)` | Callable | Stash settings so they survive the lobby→match hop. |
| `int32 GetNumFoundRooms() const` | Pure | Count from the last `FindRooms`. |
| `FString GetRoomName(int32 SearchResultIndex) const` | Pure | Display name of a found room. |
| `int32 GetRoomOpenSlots(int32 SearchResultIndex) const` | Pure | Open slots of a found room. |

Properties (`EditDefaultsOnly, BlueprintReadWrite`): `LobbyMapURL`
(default `/Game/Maps/Core/L_Lobby`), `LobbyGameModeURL`
(`/Script/PuttBattle.PBLobbyGameMode`), `MainMenuMapURL` (where clients go if the
host drops). Also broadcasts completion delegates (Create/Find/Join/Destroy) and
a host-left signal; `HandleNetworkFailure` gates on `NM_Client` so per-client
drops on the host don't false-trigger a teardown.

---

## UI

### `UPBHUDWidget` — [`UI/PBHUDWidget.h`](../Source/PuttBattle/UI/PBHUDWidget.h) / [`.cpp`](../Source/PuttBattle/UI/PBHUDWidget.cpp)

`UUserWidget` base for `WBP_HUD`. Does the **data plumbing** in C++ (finds the
local ball + cup, binds delegates) and forwards everything to
`BlueprintImplementableEvent`s the WBP graph implements as visuals
([CONVENTIONS §1/§2](CONVENTIONS.md) — UI never computes game truth).

| Signature | Kind | What it does |
|---|---|---|
| `void OnStrokesChanged(int32 NewStrokeCount)` | Event (implement) | **BP draws the stroke counter.** |
| `void OnAimPower(float Power01, bool bIndicatorHidden)` | Event (implement) | **BP animates the power bar** (hidden when Jumbled). |
| `void OnAimEnded()` | Event (implement) | **BP hides the power bar.** |
| `void OnLocalBallSunk()` | Event (implement) | **BP shows the sunk toast.** |

`NativeTick` includes belt-and-suspenders re-binding for late-replicating
PlayerState on remote clients, and re-binds on pawn re-possession (respawn).

---

## Placeholders

Tiny `UObject` stubs that only reserve a subsystem folder
([CONVENTIONS §1](CONVENTIONS.md)) until the real system ships. **No logic, no BP
surface. Delete each one the moment a real class lands in its folder.**

- `UPBPowerupsPlaceholder` — [`Powerups/PBPowerupsPlaceholder.h`](../Source/PuttBattle/Powerups/PBPowerupsPlaceholder.h) → real framework in plans/05.
- `UPBProgressionPlaceholder` — [`Progression/PBProgressionPlaceholder.h`](../Source/PuttBattle/Progression/PBProgressionPlaceholder.h) → plans/09.
- `UPBSpectatePlaceholder` — [`Spectate/PBSpectatePlaceholder.h`](../Source/PuttBattle/Spectate/PBSpectatePlaceholder.h) → plans/08.

---

## Tests

Automation Spec tests for **pure logic only** (no physics scene, no network),
gated by `WITH_DEV_AUTOMATION_TESTS` and runnable from the Session Frontend or
`-ExecCmds="Automation RunTests PuttBattle.Tests…"`.

- `FPBShotValidationTest` — [`Tests/PBShotValidationTest.cpp`](../Source/PuttBattle/Tests/PBShotValidationTest.cpp).
  Registered `PuttBattle.Tests.Shot.RequestValidation`. Exercises
  `UPBShotComponent::IsShotRequestStructurallyValid()` — honest payloads pass,
  forged ones (power>1, negative, NaN, Inf) reject.
- `FPBSurfaceRollDistanceTest` — [`Tests/PBSurfaceRollTest.cpp`](../Source/PuttBattle/Tests/PBSurfaceRollTest.cpp).
  Registered `PuttBattle.Tests.Surfaces.RollDistanceOrdering`. Integrates
  `ComputeRollDeceleration` and asserts roll distances order Ice > Fairway > Sand
  > Sticky (Ice ≥ 4× Sticky).

---

## Module root

- [`PBCollisionChannels.h`](../Source/PuttBattle/PBCollisionChannels.h) — a
  `namespace PBCollision` of `constexpr` aliases mirroring the channel
  assignments in `DefaultEngine.ini` (the .ini is the source of truth;
  [CONVENTIONS §4](CONVENTIONS.md)). See [Appendix B](#appendix-b--collision-channels).
- [`PuttBattle.h`](../Source/PuttBattle/PuttBattle.h) / [`.cpp`](../Source/PuttBattle/PuttBattle.cpp)
  — module boilerplate (`IMPLEMENT_PRIMARY_GAME_MODULE`). No game code.

---

## Appendix A — Console variable (`pb.*`) master list

Live-tweakable in PIE via the console (backtick `` ` ``). Float CVars default to
`-1` meaning "use the authored UPROPERTY"; any value `≥ 0` overrides it live
([LEARNING.md](LEARNING.md), "Console variables"). Registered in the listed file.

| CVar / command | File | Effect |
|---|---|---|
| `pb.Ball.LinearDamping` | `PBBallPawn.cpp` | Override linear damping. |
| `pb.Ball.AngularDamping` | `PBBallPawn.cpp` | Override angular damping. |
| `pb.Ball.MaxImpulse` | `PBBallPawn.cpp` | Override full-power impulse. |
| `pb.Ball.MaxSpeed` | `PBBallPawn.cpp` | Override planar speed cap. |
| `pb.Ball.Dump` | `PBBallPawn.cpp` | Log effective feel values + attributes. |
| `pb.Ball.SetScale <f>` | `PBBallPawn.cpp` | Set `ScaleMultiplier` + re-apply (mass/visuals). |
| `pb.Ball.SetMass <kg>` | `PBBallPawn.cpp` | Set `BaseMassKg` live. |
| `pb.Ball.Invert [0\|1]` | `PBBallPawn.cpp` | Toggle `bAimInverted` (Reverse test). |
| `pb.Ball.Lock [0\|1]` | `PBBallPawn.cpp` | Toggle `bShotLocked` (Lock test). |
| `pb.Ball.HideAim [0\|1]` | `PBBallPawn.cpp` | Toggle `bAimIndicatorHidden` (Jumble test). |
| `pb.Shot.MaxDragFraction` | `PBShotComponent.cpp` | Override max drag (fraction of viewport height). |
| `pb.Shot.DeadZone` | `PBShotComponent.cpp` | Override release-cancel dead-zone (px). |
| `pb.Shot.AtRestSpeed` | `PBShotComponent.cpp` | Override at-rest speed threshold. |
| `pb.Shot.RollTimeout` | `PBShotComponent.cpp` | Override roll force-stop timeout. |
| `pb.Shot.DrawDebugPreview` | `PBShotComponent.cpp` | Draw debug aim line while aiming. |
| `pb.Surface.DragCoefficient` | `PBSurfaceSamplerComponent.cpp` | Override global roll-drag coefficient. |
| `pb.Camera.AimClickScale` | `PBPlayerController.cpp` | Clickable radius around ball (press inside = aim). |
| `pb.Camera.OrbitSpeed` | `PBPlayerController.cpp` | Override RMB orbit yaw (deg/px). |
| `pb.Camera.FaceHoleSpeed` | `PBPlayerController.cpp` | Cam-2 face-hole auto-yaw easing speed. |

---

## Appendix B — Collision channels

Defined in `DefaultEngine.ini`, aliased in
[`PBCollisionChannels.h`](../Source/PuttBattle/PBCollisionChannels.h). **Every
static mesh in every hole variant must be `PB_Floor` or `PB_Wall` — never a
default channel** ([CONVENTIONS §4](CONVENTIONS.md)); Ghost Ball and rest-validity
depend on it.

| Channel | Alias | Meaning |
|---|---|---|
| `PB_Ball` → `ECC_GameTraceChannel1` | `PBCollision::Ball` | Balls. Profile `PB_BallProfile`: ignores `PB_Ball` (ghost balls), blocks Floor + Wall, overlaps triggers. |
| `PB_Floor` → `ECC_GameTraceChannel2` | `PBCollision::Floor` | Every surface a ball may roll/rest on (the surface sampler traces this). |
| `PB_Wall` → `ECC_GameTraceChannel3` | `PBCollision::Wall` | Blocking surfaces a ball bounces off but never rests on (Ghost Ball phases these). |

---

## Appendix C — Key pipelines (how the pieces talk)

**Shot** — `APBPlayerController` LMB drag → `UPBShotComponent::StartAim/UpdateAim`
(client previews via `OnUpdateAimPreview`) → `ReleaseAim` fires `OnLocalShotFired`
(local cosmetics) then `Server_RequestShot` (or runs directly on the listen-server
host) → `_Validate` (cheat guard) → `ServerShotStateAllows` (phase/at-rest/lock/
finished) → `ExecuteShot` (server impulse + `APBPlayerState::AddStroke`).

**Respawn** — hazard overlap / `FellOutOfWorld` (KillZ) → (authority)
`APBGameMode::RespawnAtCheckpoint(Ball)` → ball teleports to the highest
`APBCheckpointActor` recorded on `APBPlayerState` (or `APBTeePad`), zero velocity,
no penalty. Cosmetics fire via the actors' local Events.

**Sink** — `APBCupActor` Tick (authority) re-checks overlapping balls (speed +
scale + radius) → `AcceptBall` stops the ball, broadcasts `OnBallSunk`, calls
`APBMatchGameMode::NotifyBallSunk` → stamps `APBPlayerState` finish order/time →
player → spectate.

**Match phases** — `APBMatchGameMode` (server) owns transitions via one
`FTimerHandle`, writes `EPBMatchPhase` + a server-time deadline onto
`APBMatchGameState`; clients read `GetMatchPhase()` / `GetPhaseTimeRemaining()`
(drift-free) and bind `OnMatchPhaseChanged` to gate input + drive UI.

**Lobby → match** — host `CreateRoom` (`UPBSessionSubsystem`) → lobby map +
`APBLobbyGameMode`; players `Server_SetReady`; host `Server_RequestStartMatch` →
GameMode `ServerTravel` (`?game=…?listen`) carrying `FPBMatchSettings` through the
session subsystem (the GameState does not survive travel; PlayerState does via
`CopyProperties`).

---

## Rules for future agents

**This file is a derived index. Keep it true or delete the stale line — a wrong
map is worse than no map.** When you add, rename, move, or delete a C++ file, or
change a class's Blueprint-exposed surface, **update this glossary in the same
commit** (same discipline as [CONVENTIONS §8.5](CONVENTIONS.md) for tags/channels).

1. **Trigger.** Update this doc whenever you: add/rename/move/delete a file under
   `Source/PuttBattle/`; add/remove/rename a `UFUNCTION` with a Blueprint
   specifier, a `BlueprintReadWrite/ReadOnly` `UPROPERTY`, a `BlueprintAssignable`
   delegate, an RPC, or a `pb.*` CVar; or change what a class fundamentally does.
   A pure-internal C++ refactor that touches no file boundary and no BP/RPC/CVar
   surface needs no edit here.

2. **Place it right.** One section per folder, matching the
   [Folder map](#folder-map-one-subsystem-per-folder). New subsystem folder → new
   `##` section **and** a new Folder-map row. Group `.h` + `.cpp` of one class in
   a single entry (note "`.cpp` implements it").

3. **Per-class entry shape** (keep it consistent):
   - One-line class id: `` `ClassName` `` + UE base (e.g. `UCLASS : APawn`) +
     clickable `.h`/`.cpp` links.
   - 1–3 sentence **purpose** — prefer *why it exists* over restating signatures.
   - **Blueprint functions** table (`Signature | Kind | What it does`) — see the
     [legend](#how-to-read-the-blueprint-columns) for the Kind values.
   - **Key Blueprint properties** — list the `EditAnywhere` feel-tuning ones and
     component refs; you need not list every trivial field.
   - **RPCs / Network / CVars / notes** where they exist (authority, replication,
     gotchas, BP-subclass contract).

4. **Accuracy is the whole point — ground every claim in a real macro.** Before
   listing a member as BP-exposed, confirm the actual specifier. The fastest
   check before editing this doc:
   ```
   rg -n "BlueprintCallable|BlueprintPure|BlueprintImplementableEvent|BlueprintNativeEvent|BlueprintAssignable|BlueprintReadWrite|BlueprintReadOnly|UFUNCTION\(Server|UFUNCTION\(Client|UFUNCTION\(NetMulticast|TAutoConsoleVariable|FAutoConsoleCommand" Source/PuttBattle/<Folder>
   ```
   If a member isn't in that output, it is **not** part of the BP/RPC/CVar
   surface — don't list it as such. Never invent a function that "should" exist.

5. **Don't duplicate the other docs.** Engine *concepts* (what a `UWorldSubsystem`
   is, replication, CVars) belong in [docs/LEARNING.md](LEARNING.md) — link them
   here, don't re-explain. *Design rationale* (the D-numbers, DF-numbers) belongs
   in [docs/README.md](README.md) / [docs/DECISIONS.md](DECISIONS.md) — cite the
   number, don't restate the argument. This file answers only **"what is this
   file and what can I touch from Blueprint?"**

6. **Placeholders.** When a real class replaces a `UPB*Placeholder`, delete the
   placeholder file *and* its line in the [Placeholders](#placeholders) section,
   and add a proper entry for the new class.

7. **New CVars / channels** also go in the relevant Appendix table
   ([A](#appendix-a--console-variable-pb-master-list) /
   [B](#appendix-b--collision-channels)); a new cross-subsystem flow worth tracing
   goes in [Appendix C](#appendix-c--key-pipelines-how-the-pieces-talk).

8. **Stamp the top.** Update the "Last regenerated against …" line when you do a
   broad pass, so the next reader knows how fresh the map is.

9. **Regenerating from scratch** (after a big phase lands): re-derive rather than
   trust this file — read each subsystem folder and rebuild the entry from the
   real macros. A many-file sweep is a good fit for a fan-out (one reader per
   folder, each extracting purpose + BP surface, then verify the surface against
   the grep above). The current file was built that way.
