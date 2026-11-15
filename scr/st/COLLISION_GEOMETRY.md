# Level collision geometry data

The level streaming code and the collision query share level records with
other systems. This document distinguishes the data path visible in the
recompiled code from the still-undecoded asset format.

## Loading the level table

`ST_UpdateLevelStreaming` is a multi-stage level streaming state machine,
called from the main loop and other level-transition paths. In its initial
resource stage it requests a resource indexed from the current level into the
staging buffer at `0x80088C88` through `sub_80015D54` (which uses `ST_GetIndexedResourceLocation`). The asynchronous
callback `ST_OnLevelDataChunkLoaded` advances the streaming stage. At the
next stage the state machine starts another resource request and uses
`sub_80014CFC` to copy `0x3E00` 32-bit words from that staging buffer to
`0x8017BED0` (a byte count of `0xF800`). The copied region includes the
12-byte records at `0x8017BED0` and the coordinate-indexed tables beginning
at `0x801856D0` used by collision candidate selection.

The copy routine is a generic word copy; the resource also serves other
level systems. The exact resource name, complete file layout, and how the
separate shared geometry resource is populated have not yet been established.
Do not assume this buffer is a standalone collision mesh.

## Querying collision geometry

1. `ST_PlayerMoveWithCollision` builds a query and calls
   `ST_RunCollisionQuery`. When the relevant query flag is set, it calls
   `ST_QueryLevelGeometry`.
2. `ST_QueryLevelGeometry` reads a shared level-data pointer from
   `0x8016539C` and calls `ST_SelectSpatialCandidates`. Selection converts
   coordinates to cell indices using division by 4096, then intersects
   coordinate-indexed bitsets to form a list of candidate record indices.
   Negative coordinates are rounded toward zero by the instruction sequence.
3. `ST_DecodeCandidateOrigins` resolves each selected index into a 12-byte
   record at `0x8017BED0 + 12 * index` and writes a 16-byte output entry.
   The decoded coordinates are:

   ```text
   x = record[8] << 12
   y = (record[9] << 12) + ((record[11] - 128) << 7)
   z = record[10] << 12
   ```

   The bytes are read unsigned; the subtraction above centers byte 11.
   These are stored coordinate units. The code does not identify their
   physical units or establish a Q12.4 convention.
4. The geometry query invokes `sub_80046604`, `sub_800468C0`, and
   `sub_80047884` to prepare and test candidate geometry. The latter two
   access the shared level-data pointer and derive subrecord addresses using
   fields from the selected 12-byte records. Their full formats and exact
   individual responsibilities need further investigation.
5. Results, including flags and distance-like values, are written to the
   shared query structure. `ST_PlayerMoveWithCollision` uses them to choose
   and adjust the displacement; see `PLAYER_MOVEMENT.md`.

If the candidate search is empty, `ST_QueryLevelGeometry` writes `0x80000`
to two query-result fields. This sentinel participates in the player's
vertical correction; it is not a decoded surface distance.
