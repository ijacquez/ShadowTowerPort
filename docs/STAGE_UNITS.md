# Stage 0 geometry and world-coordinate scale

## Stage data

`FDAT.T` begins with a 16-bit count followed by 16-bit sector offsets. The
engine copies the offsets following the count and indexes a stage's resources
at `stage * 10 + slot`. For stage 0, slot 1 occupies sectors 18..49
(65,536 bytes); slot 5 occupies sectors 93..103 (22,528 bytes). The level
streaming stages in `ST_UpdateLevelStreaming` load slot 1 into the staging
buffer and copy the first `0xF800` bytes into the level-record region, then
load slot 5 at `0x800F2C88`; `sub_80044EE8` sets the shared geometry pointer
to that address. The stage 0 model archive starts with ID `0x41`, flags 0,
and 256 40-byte model records. Following the engine's address calculations,
each record's first word is the vertex offset relative to archive offset 12,
its second word is the vertex count, and each vertex is three signed
16-bit coordinates plus a 16-bit padding field. Stage 0 contains 610 vertices
across these slots. The 512 placement records include unused records.

## Scale established by actual data and code

- `Extracted/ST/FN` begins with floor 0 and player appearance coordinates
  X=0, Y=2, Z=-1. `ST_LoadInitialGameState` adds 64 to each value, shifts
  left 12, and subtracts 1600 from Y. The resulting stored player position is
  `(262144, 268736, 258048)`.
- Stage 0 placement record 3 decodes to `(262144, 270336, 258048)` and
  references model 2. Thus its origin matches the initial player X/Z exactly,
  and its stored Y is exactly 1600 greater than the player Y. This establishes
  a shared scale and reference offset; it does not by itself identify a floor
  surface or the player's full height.
- `ST_DecodeCandidateOrigins` converts each record's coordinate byte with a
  left shift of 12. `ST_SelectSpatialCandidates` indexes its spatial tables
  by coordinate divided by 4096. One spatial cell is therefore **4096 stored
  position units** along each axis. The additional vertical placement byte
  uses steps of 128 stored units.
- Stage 0 model 2 contains vertices at `(-1024,-512,-2048)` and
  `(1024,-512,-2048)`, a horizontal separation of 2048 model units: half a
  cell. In `ST_TransformCandidateVertices`, those signed vertex coordinates are fed into the
  GTE with a rotation matrix and the decoded placement origin as translation.
  The transformed result is compared directly to the player position. The
  matrix uses 4096 for one and the GTE shifts the product by 12, so **one
  model-coordinate unit equals one stored world-position unit**, apart from
  rotation and integer rounding. It is not multiplied by 16 during placement.
- Model 2's four-vertex collision record at stage resource offset `0x29DC`
  has flag halfword `0x0001` and references vector `(0, -4096, 0)`. Its
  four vertices are `(-1024, -1024, -2048)`, `(-1024, -1024, 2048)`,
  `(1024, -1024, -2048)`, and `(1024, -1024, 2048)`. Placement record 3
  selects rotation 1, which leaves Y unchanged. Its transformed plane is
  therefore at stored Y `270336 - 1024 = 269312`, with its horizontal
  extent containing the initial player X/Z. Since positive world Y is down,
  that plane is 576 stored units below the initial player Y of 268736.
  The record format and transformation are described in
  [COLLISION_GEOMETRY.md](COLLISION_GEOMETRY.md).

Using one spatial cell as one world unit, positions and movement displacements
have 12 fractional bits: divide stored values by 4096. A cell is 1.0, the
model edge above is 0.5, and the player Y offset is 0.390625. The default
movement limit is 192/4096 = 0.046875 units per update, and its default
acceleration step is 48/4096 = 0.01171875 units per update.

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
