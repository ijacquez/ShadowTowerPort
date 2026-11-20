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
