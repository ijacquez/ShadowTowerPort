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
`ST_CopyWords` to copy `0x3E00` 32-bit words from that staging buffer to
`0x8017BED0` (a byte count of `0xF800`). The copied region includes the
12-byte records at `0x8017BED0` and the coordinate-indexed tables beginning
at `0x801856D0` used by collision candidate selection.

For stage 0 these resources are entries 1 and 5 of `ST/COM/FDAT.T`: entry 1
contains the copied level table, while entry 5 is the shared model archive
loaded at `0x800F2C88`. The resource also serves other level systems; the
full model and collision formats have not yet been established. See
[STAGE_UNITS.md](STAGE_UNITS.md) for the archive offsets and coordinate scale.

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
   These are stored coordinate units; model vertices and player positions use
   the same scale. A spatial cell is `0x1000` stored units (1.0 cell). Their physical units
   are not established; see [STAGE_UNITS.md](STAGE_UNITS.md).
4. `ST_TransformCandidateVertices` uses each selected record's byte 0 to
   select a 40-byte entry at `level_data + 12`. The entry's word at offset 0
   locates an eight-byte-stride vertex array at `level_data + offset + 12`,
   and its word at offset 4 gives the vertex count. The routine selects a
   matrix using the low two bits of record byte 3, and uses the decoded
   origin as the GTE translation. For every vertex it writes the transformed
   coordinates, then writes three more vectors by subtracting the query's
   current position, proposed position, and position with Y adjusted by
   `ST_COLLISION_QUERY_VERTICAL_PARAMETER - 0x403`, respectively. The outputs use 16 bytes per vector.
   The routine exits if the 32-bit calculated end of the first output is
   greater than its buffer limit in an unsigned comparison.
5. The geometry query invokes `ST_TestThreeVertexCollisionRecords` and `ST_TestFourVertexCollisionRecords` to test
   candidate geometry. In `ST_TestThreeVertexCollisionRecords`, let `entry` be the selected
   40-byte entry and `base` the shared level-data pointer. It reads an
   unsigned halfword count at `entry + 0x20` and iterates 12-byte records
   beginning at:

   ```text
   base + 12 + word(entry + 0x10)
       + 20 * halfword(entry + 0x18) + 24 * halfword(entry + 0x1A)
       + 28 * halfword(entry + 0x1C) + 32 * halfword(entry + 0x1E)
   ```

   The halfwords in this expression are unsigned. For each such record,
   its unsigned halfword at offset 4 is added to `base + 12 + word(entry +
   8)` to address a vector loaded into the GTE and transformed with the
   selected record's matrix. The three unsigned halfwords at record offsets
   `6`, `8`, and `10` select transformed vertices: each halfword is doubled
   and added to each transformed-vertex array's base address.
   `ST_TestFourVertexCollisionRecords` starts at the same calculated
   address plus `12 * halfword(entry + 0x20)` and iterates
   `halfword(entry + 0x24)` records with a 16-byte stride. Both count
   halfwords are unsigned. It uses the same three vertex offsets and a
   fourth at record offset `12`. Both helpers compute cross products from
   the selected transformed vertices and compare the results against the
   transformed vector. **OPEN:** the remaining record fields and complete
   collision result meanings are not established.
6. Results, including flags and distance-like values, are written to the
   shared query structure. `ST_PlayerMoveWithCollision` uses them to choose
   and adjust the displacement; see [PLAYER_MOVEMENT.md](PLAYER_MOVEMENT.md).

If the candidate search is empty, `ST_QueryLevelGeometry` writes `0x80000`
to two query-result fields. This sentinel participates in the player's
vertical correction; it is not a decoded surface distance.
