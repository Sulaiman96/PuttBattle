# Phase 7 (ahead) — Hole 3 "The Cake Stand" graybox · `V_A`

Single graybox hole built ahead of the Phase-7 variant **system** (T7.1 definitions / T7.2
validator are not yet implemented). It is the **open putting-duel green** that plans/07 T7.3
calls for. Source design: `Sugar Rush — 5 Holes Top-Down` doc, Hole 3 ("3A The Hub", PAR 3).

Map: **`/Game/Maps/Holes/H_03_CakeStand/V_A`** (`Content/Maps/Holes/H_03_CakeStand/V_A.umap`, untracked/new).

## Status of this pass
The layout was greyboxed by an earlier session (the `.umap` was already populated and saved).
This pass **verified it end-to-end** against the design doc, CONVENTIONS §4 (collision contract),
and the plans/07 T7.2 actor manifest; normalized the Lolly beacon's collision state to a single
consistent value; re-saved; and documented the result + the one open contract item below. No
geometry/actor placement was changed — the one defect found (Lolly collision channel) **cannot be
fixed through the currently-active editor MCP** (see "Requires editor / old MCP").

## Layout (px→cm at 1 px = 10 cm, anchored so the cup sits at world origin)

| Element | Actor / mesh | Transform (cm) | Channel | Surface |
|---|---|---|---|---|
| Outer green (cake disc) | `Floor_GreenBase` — Cylinder | (0,0,−10), scale 36 → **r ≈ 18 m**, top z=0 | PB_Floor | Fairway (`PM_Surface_Fairway` + `M_Surface_Fairway`) |
| Sherbet ring | `Floor_SandRing` — Cylinder | (0,0,−9), scale 20.2 → r ≈ 10.1 m, top z=+1 | PB_Floor | Sand |
| Central hub | `Floor_HubGreen` — Cylinder | (0,0,−8), scale 14.4 → r ≈ 7.2 m, top z=+2 | PB_Floor | Fairway |
| Hub lip (4 arcs, gaps at N/E/S/W) | `LipWall_NE/NW/SW/SE` — Cube | radius ≈ 8.5 m on the diagonals, 11 m × 0.3 × 0.5, z=25 | PB_Wall | — (untextured greybox) |
| Cup | `PBCupActor` "Cup" | (0,0,2) on the hub; defaults (sink 120, balloonMax 1.3, catch 12) | — | — |
| Flag beacon | `Lolly_Pole` + `Lolly_Head` — Cylinders | at origin, pole z 0→220, head z≈230 | ⚠ WorldStatic (see open item) | — |
| Tees ×4 | `PBTeePad` N/E/S/W | (±1390,0) / (0,±1420), z=20, each yawed to face the cup | — | — |
| Checkpoints ×2 | `PBCheckpointActor` idx 0 / idx 1 | (−650,−1000,30) and (650,1000,30) — opposite arcs | — | — |
| Cameras ×2 | `PBCameraRig` Frame (prio 1) / Overview (prio 2, auto-orbit) | Frame (−3200,0,2400) pitch −37; Overview orbits centre | — | — |
| Lighting | DirectionalLight "Sun" + SkyLight + SkyAtmosphere | — | — | — |
| World | `WorldSettings` | KillZ = −500; DefaultGameMode = `BP_PBGameMode` | — | — |

**Stacking trick:** the three Fairway/Sand discs are concentric with 1 cm height steps
(green top z=0 < sand top z=+1 < hub top z=+2), so the sand reads as a *ring* (visible 7.2–10.1 m)
and the hub as a raised fairway centre, from a single solid disc each. The 4 lip-wall arcs sit on
the diagonals, leaving **cardinal gaps that line up with the 4 tees** — so each tee's straight putt
fires through its own gap at the cup, exactly the design's "fire straight through a hub gap" hero line.

## Design fidelity
- **Hero** (slow "drop-speed" putt straight through a gap → one-putt) and **Safe** (lay up on the
  sherbet ring outside the lip, then tap in) routes are both supported by the geometry + the Sand
  band slowing approach shots.
- **Flag** = the Big Lolly beacon on the hub centre, visible from all four tees (the design's stated role).
- Goodie-jar pickups are intentionally **omitted** — no pickup actor exists until Phase 5
  (matches the established greybox recipe).

## Verification
- Full property inventory of every actor read back via MCP: all three floors confirmed on
  **PB_Floor** (`ECC_GameTraceChannel2`), profile `Custom`, `QueryAndPhysics`, correct
  `PhysMaterialOverride` (Fairway/Sand) + render material; the 4 lip walls on **PB_Wall**
  (`ECC_GameTraceChannel3`); tees face centre; cup on the hub; checkpoints indexed 0/1; cameras
  prioritised 1/2 with one auto-orbit; KillZ + GameMode set.
- Viewport captures (top-down + oblique + the Frame-camera gameplay pose) confirm the hub/ring/lip
  geometry, the cardinal gaps, and actor placement.

## Open items / known issues
1. **Lolly beacon violates CONVENTIONS §4** — `Lolly_Pole`/`Lolly_Head` are on `ECC_WorldStatic`
   (a default channel), not `PB_Floor`/`PB_Wall`. The active editor MCP **cannot** change a
   component's collision channel/enabled (see below), so this is left as-is and flagged.
   Gameplay impact is minor: the cup's 12 cm catch radius > the 3 cm pole, so slow "drop-speed"
   putts are caught before reaching the pole; only fast (non-sinkable) shots would deflect off the
   flagstick — which actually suits the "chaos bowl" theme. **Recommended fix:** set the two Lolly
   components to **PB_Wall** (strict §4 compliance, design-consistent central obstacle) — or
   **NoCollision** if playtesting shows the deflection feels unfair.
2. **Overexposed surfaces** — in the editor viewport the candy floors render blown-out white (only
   the green rim shows). This uses the same `M_Surface_*` materials + lighting recipe as every other
   hole, so it is almost certainly a **project-wide** exposure characteristic, not specific to Hole 3
   (confirm by opening `H_Test/V_A`). Recommend a shared exposure solution (an unbound
   PostProcessVolume with clamped/manual auto-exposure) applied course-wide rather than per hole.
3. **4 tees vs the T7.2 manifest's "1 TeePad (with 6 spawn offsets)".** This hub hole deliberately
   uses four converging tees (the design shows T1–T4) to match its "six balls converge on the
   cherry-on-top" concept. Decision for the human: keep four distinct tees (needs the Phase-7 spawn
   logic to seat ≤6 players across them) vs collapse to one tee with offsets.
4. **Checkpoints sit on the diagonals (SW/NE)** while the tee→cup hero lines are cardinal, so they
   don't sit on any single tee's line — fine for respawn coverage on this radial hole, but noted.

## Requires editor / old MCP
The active MCP is the **native UE 5.8 toolset server** (`.mcp.json` → `unreal-mcp`, HTTP :8000).
Its `ObjectTools.set_properties` can write a component's collision **profile name** string but
**not** the resolved `FBodyInstance` fields (`collisionEnabled`, `objectType`, responses) — those
need `FBodyInstance`'s setters. Setting a PB_Floor/PB_Wall/NoCollision channel therefore requires
either the **disabled node-based `ue-mcp`** (`_disabledServers` in `.mcp.json`, whose
`editor execute_python` calls `set_collision_object_type`/`set_collision_enabled`) or the editor
Details panel. Item 1 above is the only thing this leaves outstanding for the map.
