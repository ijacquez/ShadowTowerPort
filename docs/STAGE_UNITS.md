# Stage 0 geometry and world-coordinate scale

## Stage data

`FDAT.T` begins with a 16-bit count followed by 16-bit sector offsets. The
engine copies the offsets following the count and indexes a stage's resources
at `stage * 10 + slot`. For stage 0, slot 1 occupies sectors 18..49
(65,536 bytes); slot 5 occupies sectors 93..103 (22,528 bytes). The level
streaming stages in `ST_UpdateLevelStreaming` load slot 1 into the staging
buffer and copy the first `0xF800` bytes into the level-record region, then
load slot 5 at `0x800F2C88`; `ST_StartLevelStreamingAndWait` sets the shared geometry pointer
to that address. The stage 0 model archive starts with ID `0x41`, flags 0,
and 256 40-byte model records. Following the engine's address calculations,
each record's first word is the vertex offset relative to archive offset 12,
its second word is the vertex count, and each vertex is three signed
16-bit coordinates plus a 16-bit padding field. Stage 0 contains 610 vertices
across these slots. The 512 placement records include unused records.

## Scale established by actual data and code

- `Extracted/ST/FN` begins with floor 0 and player appearance coordinates
  X=0, Y=2, Z=-1. `ST_LoadInitialGameState` adds 64 to each value, shifts
  left 12, and subtracts `0x640` (0.390625 cells) from Y. The resulting stored player position is
  `(262144, 268736, 258048)`, or `(0x40000, 0x419C0, 0x3F000)`
  (64.0, 65.609375, 63.0) in spatial-cell units.
- Stage 0 placement record 3 decodes to `(0x40000, 0x42000, 0x3F000)`
  (64.0, 66.0, 63.0 cells) and
  references model 2. Thus its origin matches the initial player X/Z exactly,
  and its stored Y is exactly `0x640` (0.390625 cells) greater than the player Y. This establishes
  a shared scale and reference offset; it does not by itself identify a floor
  surface or the player's full height.
- `ST_DecodeCandidateOrigins` converts each record's coordinate byte with a
  left shift of 12. `ST_SelectSpatialCandidates` indexes its spatial tables
  by coordinate divided by 4096. One spatial cell is therefore **4096 stored
  position units** along each axis. The additional vertical placement byte
  uses steps of `0x80` (0.03125 cells).
- Stage 0 model 2 contains vertices at `(-0x400, -0x200, -0x800)` and
  `(0x400, -0x200, -0x800)` ((-0.25, -0.125, -0.5) and
  (0.25, -0.125, -0.5) cells), a horizontal separation of `0x800` (0.5
  cells). In `ST_TransformCandidateVertices`, those signed vertex coordinates are fed into the
  GTE with a rotation matrix and the decoded placement origin as translation.
  The transformed result is compared directly to the player position. The
  matrix uses `0x1000` (1.0) for one and the GTE shifts the product by 12, so **one
  model-coordinate unit equals one stored world-position unit**, apart from
  rotation and integer rounding. It is not multiplied by 16 during placement.
- Model 2's four-vertex collision record at stage resource offset `0x29DC`
  has flag halfword `0x0001` and references vector `(0, -0x1000, 0)`
  (0.0, -1.0, 0.0) in the GTE's 12-fractional-bit scale. Its
  four vertices are `(-0x400, -0x400, -0x800)`, `(-0x400, -0x400, 0x800)`,
  `(0x400, -0x400, -0x800)`, and `(0x400, -0x400, 0x800)` (X ±0.25,
  Y -0.25, Z ±0.5 cells). Placement record 3
  selects rotation 1, which leaves Y unchanged. Its transformed plane is
  therefore at stored Y `0x42000 - 0x400 = 0x41C00` (65.75 cells), with its horizontal
  extent containing the initial player X/Z. Since positive world Y is down,
  that plane is `0x240` (0.140625 cells) below the initial player Y of
  `0x419C0` (65.609375 cells).
  The record format and transformation are described in
  [COLLISION_GEOMETRY.md](COLLISION_GEOMETRY.md).

Model 2 has no three-vertex collision records and has four four-vertex
records. Their local coordinates and vectors, before the placement rotation
and translation, are:

| Resource offset | `ST_GEOMETRY_RECORD_FLAGS` | Referenced vector in Q.12 | Plane and vertex extent in cells |
| --- | --- | --- | --- |
| `0x29BC` | `0x4` | `(0, 0x1000, 0)` (0, 1.0, 0) | Y = `-0x200` (-0.125); X = `±0x400` (±0.25); Z = `±0x800` (±0.5) |
| `0x29CC` | `0xA` | `(0x1000, 0, 0)` (1.0, 0, 0) | X = `0x400` (0.25); Y = `-0x400` to `-0x200` (-0.25 to -0.125); Z = `±0x800` (±0.5) |
| `0x29DC` | `0x1` | `(0, -0x1000, 0)` (0, -1.0, 0) | Y = `-0x400` (-0.25); X = `±0x400` (±0.25); Z = `±0x800` (±0.5) |
| `0x29EC` | `0xA` | `(-0x1000, 0, 0)` (-1.0, 0, 0) | X = `-0x400` (-0.25); Y = `-0x400` to `-0x200` (-0.25 to -0.125); Z = `±0x800` (±0.5) |

These are the records selected through model 2's geometry fields. Their
presence alone does not determine which record a movement query selects.

Using one spatial cell as one world unit, positions and movement displacements
have 12 fractional bits: divide stored values by 4096. A cell is `0x1000`
(1.0), the model edge above is `0x800` (0.5), and the player Y offset is
`0x640` (0.390625). The default movement limit is `0xC0` (0.046875 units
per update), and its default acceleration step is `0x30` (0.01171875 units
per update). The 576-unit initial plane separation is `0x240` (0.140625).

The collision routine adds the stored movement displacement directly to the
stored player position, and compares geometry transformed into the same
coordinate scale. If position used 12 fractional bits and displacement used
four, displacement would need an eight-bit left shift before that addition;
no such shift appears in this path. Interpreting both as four-fractional-bit
values is a consistent rescaling, but does not establish different formats
for position and movement.

The data establishes 4096 stored units per cell, not the original developers'
name for the unit or a physical length in metres or centimetres. The 12-bit
fraction on GTE rotation coefficients is a separate fixed-point scale: its
12-bit product shift preserves the coordinate scale rather than defining it.
