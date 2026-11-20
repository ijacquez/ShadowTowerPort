# File and resource format readers

The game code is recompiled from MIPS and addresses emulated RAM through
`MEM_*`. Names below describe behavior visible in the code; they are not
claims about original source names. The file extensions and complete formats
of the game-specific resources have not been established.

## Initial-state text

`ST_LoadInitialGameState` allocates a buffer and calls `ST_ReadDiscFile` to
read a disc file into it. It then calls `ST_ReadFixedWidthDecimalLine` for
successive fields. The reader takes a pointer to a cursor and a field width,
walks the specified bytes from right to left, accumulates decimal digits with
powers of ten, recognizes `-`, ignores other characters, and advances the
cursor past the next LF. It has no bounds checking in this recompiled code.
The initial X/Y/Z fields are decoded decimal values shifted left by 12; Y
also has `0x640` (0.390625 spatial cells) subtracted. In spatial-cell units these positions have 12
fractional bits; their physical unit is unknown. See [STAGE_UNITS.md](STAGE_UNITS.md).

## Sector-indexed resources

`ST_GetIndexedResourceLocation` selects an index table from a 12-byte entry
for the requested resource group. Two adjacent unsigned 16-bit entries are
sector offsets: their difference multiplied by 2048 is the resource size in
bytes. If requested, it uses `ST_AddSectorsToCdPosition` to add the first
offset to the group's base CD position. `ST_GetIndexedResourceSize` reads
the same pair when only the byte count is needed. The routines assume the
indices and data are valid; no bounds checks are visible.

`ST_QueueIndexedResourceRead` resolves an indexed resource's CD position and
byte count with `ST_GetIndexedResourceLocation`. If the byte count is nonzero,
it passes the position, size, destination, and callback to `ST_QueueDiscRead`
with request type 2. The queue routine rounds the byte count up to 2048-byte
sectors and stores the request in a circular queue.

The associated CD position helpers decode or encode packed-BCD
minute/second/frame values: `ST_BcdToBinary`, `ST_BinaryToBcd`,
`ST_CdPositionToSector`, `ST_SectorToCdPosition`, and
`ST_AddSectorsToCdPosition`. The conversion uses 75 sectors per second and
60 seconds per minute. These operate on CD positions; the code does not label
all resulting sector counts as ISO LBAs.

## Level geometry and other formats

### Resource vectors and sequences

[ST_SampleResourceVector](../scr/st/ST_SampleResourceVector.c) obtains a
resource pointer through `sub_80033DB8`. A missing or unavailable pointer
causes it to clear three output halfwords and return 1. With a pointer, it
uses the fields named in [ST_ResourceVectorFormat.h](../scr/st/ST_ResourceVectorFormat.h).
The word `ST_VECTOR_RESOURCE_MODEL_TABLE_OFFSET` is relative to the resource
base. Each model-table record is 40 bytes; its
`ST_VECTOR_MODEL_DATA_OFFSET` word, plus `ST_VECTOR_MODEL_DATA_BIAS`, is
relative to the model-table base. Vector entries are 8 bytes.

For the unsigned-byte selector supplied by the active-record offset path,
when the selector is at least `0x80`, the sampler uses its low seven bits as
the model-table index and copies the selected eight-byte vector entry. If the
selector is below `0x80` and `ST_VECTOR_RESOURCE_SEQUENCE_COUNT` is zero,
it copies the corresponding entry from model-table record zero. Otherwise,
it uses model-table record zero and calls
[ST_SelectResourceSequenceInterval](../scr/st/ST_SelectResourceSequenceInterval.c)
with the selector and time value.

The sequence-offset table is relative to the resource base. A selected
sequence contains a byte interval count followed at
`ST_VECTOR_SEQUENCE_INTERVAL_OFFSETS` by resource-relative word offsets.
The selector adds each interval's unsigned halfword duration to a 32-bit
cumulative total and chooses the first interval whose total interpreted as
signed is greater than the signed time value. It calculates a 12-fractional-bit
interpolation factor by shifting the wrapped difference between time and the
previous cumulative total left 12, then dividing it as unsigned by the
interval duration. If the count is nonzero and no interval is selected, it
returns the last interval and factor `0x1000` (1.0). A zero count produces a
zero interval pointer and index -1; the caller assumes a valid interval pointer.

The resource's frame-offset table supplies two arrays selected by the
interval's `ST_VECTOR_INTERVAL_FIRST_FRAME` and
`ST_VECTOR_INTERVAL_SECOND_FRAME` bytes. For each of the first three
halfwords at the selected vector index, the sampler adds the base-vector
halfword and the first-frame halfword, then adds the low 32-bit product of
the signed 16-bit wrapped frame difference and interpolation factor,
arithmetic-shifted right 12. It stores the low halfword of each result. This
interpolated path writes three halfwords; the direct path copies eight bytes.
**OPEN:** the complete resource format and the in-game meaning of the
sampled vectors.

`ST_UpdateLevelStreaming` loads a level table, while
`ST_DecodeCandidateOrigins` decodes the coordinate fields of its selected
records. `ST_QueryLevelGeometry` and the geometry routines consume associated
resource data. See [COLLISION_GEOMETRY.md](COLLISION_GEOMETRY.md) for the verified path and unknowns.

The PC-side `KFCD_FindFile` in `scr/kf_cd.cpp` already has a descriptive name;
it reads an ISO 9660 primary volume descriptor and directory records.
`LoadGameEXEFromCD` reads the PS-X EXE header. Existing PS1 library routines
also include named format readers such as `_SsReadDeltaValue`. Many other
recompiled functions remain unnamed; this inventory is not proof that every
file-format reader in the game has been identified.
