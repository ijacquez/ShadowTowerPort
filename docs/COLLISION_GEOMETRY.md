# Level collision geometry data

The level streaming code and the collision query share level records with
other systems. This document distinguishes the data path visible in the
recompiled code from the still-undecoded asset format.

## Loading the level table

`ST_UpdateLevelStreaming` is a multi-stage level streaming state machine,
called from the main loop and other level-transition paths. In its initial
resource stage it requests a resource indexed from the current level into the
staging buffer at `0x80088C88` through `ST_QueueIndexedResourceRead` (which uses `ST_GetIndexedResourceLocation`). The asynchronous
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
   `ST_SPATIAL_FILTER_DESCRIPTOR_POINTER_ADDRESS`, clears
   `ST_COLLISION_QUERY_CANDIDATE_COUNT`, and calls
   `ST_SelectSpatialCandidates` with two position vectors and a null third
   vector. The selector accepts up to three position vectors. For each, it
   divides each signed coordinate by `0x1000` (1.0 cell), rounding toward
   zero, and uses the resulting X, Y, and Z indices to select one 64-byte
   bitset from each axis table. It ANDs the three bitsets for each vector,
   then ORs those intersections across the supplied vectors. It scans the
   512 resulting bits in ascending index order and appends set-bit indices
   to `ST_COLLISION_QUERY_CANDIDATE_INDICES`, incrementing
   `ST_COLLISION_QUERY_CANDIDATE_COUNT` for each accepted index.

   When the signed streaming status is 1 and
   `ST_LEVEL_STREAMING_CANDIDATE_FILTER_ENABLED` is nonzero, the selector
   accepts an index only if the corresponding
   `ST_LEVEL_PLACEMENT_RESOURCE_INDEX` is less than the unsigned
   `ST_SPATIAL_FILTER_DESCRIPTOR_LIMIT_SOURCE` word shifted right by one.
   The selector reads that word through the pointer at
   `ST_SPATIAL_FILTER_DESCRIPTOR_POINTER_ADDRESS`. **OPEN:** the in-game
   meaning of the filter and the descriptor word.
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
   select a 40-byte entry at `level_data + 12`. The entry's
   `ST_GEOMETRY_MODEL_VERTEX_OFFSET` locates an eight-byte-stride vertex array
   at `level_data + offset + 12`, and `ST_GEOMETRY_MODEL_VERTEX_COUNT` gives
   the vertex count. The routine selects a
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
   unsigned `ST_GEOMETRY_MODEL_THREE_VERTEX_COUNT` and iterates 12-byte records
   beginning at:

   ```text
   base + 12 + ST_GEOMETRY_MODEL_RECORD_OFFSET
       + 20 * ST_GEOMETRY_MODEL_20_BYTE_RECORD_COUNT
       + 24 * ST_GEOMETRY_MODEL_24_BYTE_RECORD_COUNT
       + 28 * ST_GEOMETRY_MODEL_28_BYTE_RECORD_COUNT
       + 32 * ST_GEOMETRY_MODEL_32_BYTE_RECORD_COUNT
   ```

   The expression uses the values of these fields in `entry`; the count
   halfwords are unsigned. For each record,
   `ST_GEOMETRY_RECORD_VECTOR_OFFSET` is added to
   `base + 12 + ST_GEOMETRY_MODEL_VECTOR_OFFSET` to address a vector loaded
   into the GTE and transformed with the selected record's matrix. The
   unsigned `ST_GEOMETRY_RECORD_VERTEX_0_OFFSET`, `_1_OFFSET`, and
   `_2_OFFSET` select transformed vertices: each halfword is doubled and
   added to each transformed-vertex array's base address.
   `ST_TestFourVertexCollisionRecords` starts at the same calculated
   address plus `12 * ST_GEOMETRY_MODEL_THREE_VERTEX_COUNT` and iterates
   `ST_GEOMETRY_MODEL_FOUR_VERTEX_COUNT` records with a 16-byte stride. Both
   count halfwords are unsigned. It uses the same three vertex offsets and
   `ST_GEOMETRY_RECORD_VERTEX_3_OFFSET`. Both helpers compute cross products from
   the selected transformed vertices and compare the results against the
   transformed vector. **OPEN:** the remaining record fields and complete
   collision result meanings are not established.
6. Results, including flags and distance-like values, are written to the
   shared query structure. `ST_PlayerMoveWithCollision` uses them to choose
   and adjust the displacement; see [PLAYER_MOVEMENT.md](PLAYER_MOVEMENT.md).

If the candidate search is empty, `ST_QueryLevelGeometry` writes `0x80000`
to two query-result fields. This sentinel participates in the player's
vertical correction; it is not a decoded surface distance.
