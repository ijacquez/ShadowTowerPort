# Player movement

The [game state machine](../scr/st/sub_80042D74.c) calls
`ST_PlayerUpdateTurning`, `ST_PlayerUpdateMovement`, and
`ST_PlayerMoveWithCollision` in that order in several states. The code is
recompiled MIPS: `ctx` registers and `MEM_*`
accesses represent emulated state, and comments retain original instructions.
`ST_PlayerState.h` names confirmed offsets from the player-state base
`0x80198F28`; it does not define a native C struct.

## Horizontal input

`ST_PLAYER_LOCAL_MOVE_X` (`0x2B8`) and `ST_PLAYER_LOCAL_MOVE_Z` (`0x2C0`)
are signed 32-bit input accumulators used as local X and Z components by
[ST_PlayerUpdateMovement](../scr/st/ST_PlayerUpdateMovement.c). The
control masks are initialized by [ST_InitDefaultControlBindings](../scr/st/ST_InitDefaultControlBindings.c): `0x10`
increases Z and `0x40` decreases Z; `0x800` increases X and `0x400`
decreases X. The bindings may be changed. The two opposing X controls
cancel; the positive Z branch takes precedence if both Z controls are held.

With a positive movement limit `L` and no 32-bit arithmetic wrap, a held direction changes its accumulator
by `L >> 2`, clamped to `+L` or `-L`. With no direction it approaches zero
by `L >> 3`, clamped at zero. Reversing direction uses the held-direction
step. The gameplay state normally sets L to 192, but can reduce or clear it.
For example, starting from zero with only positive Z held and L unchanged,
Z is 48, 96, 144, 192, then 192 on successive updates. On release from 192,
it decreases by 24 per update. The shifts truncate the step; for example,
`L >> 3` is zero when `L` is between 1 and 7. The default and modified
limits are set in [sub_80042D74.c](../scr/st/sub_80042D74.c).

The routine processes the X/Z magnitudes and rotates the local vector by
negative player yaw using the GTE. It also damps existing movement components
and adds other contributions. The accumulators therefore are not directly
world-position increments.

More precisely, the first GTE command loads the accumulator words into IR1
and IR3, squares their signed low halfwords, and writes the results to the
GTE MAC registers. The routine passes the 32-bit sum of those two results to
`SquareRoot0` and stores the low halfword of its result at player offset
`0x2C8`. If that halfword interpreted as signed is zero, it sets the two
horizontal vector components to zero. Otherwise, it gives each square the
sign of its original 32-bit accumulator and divides by that signed halfword.
See [ST_PlayerUpdateMovement.c](../scr/st/ST_PlayerUpdateMovement.c), the
[GTE SQR implementation](../PsyCross/src/gte/PsyX_GTE.cpp), and the
[IR register definitions](../PsyCross/include/psx/gtereg.h). The integer
divisions discard any fractional remainder.

The rotation input's Y halfword is loaded from stack offset `0x1C`; this
function does not write that offset before loading it. **OPEN:** its source
and role have not been established. At the read, it is at the caller's
incoming stack pointer minus `0x54`: the movement routine
allocates `0x70` bytes and reads offset `0x1C`. Neither the called
[SquareRoot0 wrapper](../scr/st/SquareRoot0.c) nor
[RotMatrix](../scr/st/RotMatrix.c) writes that stack location. Its value
therefore depends on the prior contents of emulated memory at that address.

Its effect on the horizontal result can be resolved for the table in the
supplied executable. The loader copies the `ST.EXE` payload to the address
in its header; in the supplied file that address is `0x80011000`, and the
payload begins at file offset `0x800`. The zero-angle word read by
[RotMatrix](../scr/st/RotMatrix.c) at `0x80082280` is therefore at file
offset `0x71A80`. Its bytes are `00 00 00 10`: the signed low halfword is
zero and the high halfword is 4096; see the [executable loader](../scr/kf_cd.cpp).

Movement passes rotation angles `(0, -yaw, 0)` to `RotMatrix`. With the
above zero-angle entry, its X/Z rows have zero in their Y-input column for
every yaw. The [GTE MVMVA implementation](../PsyCross/src/gte/PsyX_GTE.cpp)
multiplies those matrix entries by the vector's Y halfword, so that halfword
contributes zero to rotated X and Z. The subsequent angle and magnitude use
those X/Z results. This conclusion depends on that executable table entry
still containing the stated value when the function runs; the prior stack
writer and any effects outside those horizontal outputs remain **OPEN**.

The rotation uses `-yaw` and writes GTE results back to the stack before
computing another angle and magnitude. The later code updates each signed
player halfword at `0x1CC`, `0x1CE`, and
`0x1D0` by subtracting one quarter of its value, rounded toward zero. It
then clears a resulting halfword in the range -3 through 3. It adds one
quarter of the signed halfword at `0x801C74B4` to X and one quarter of the
signed halfword at `0x801C74B8` to Z, rounding toward zero. It also adds half
the signed halfwords at `0x801C0298` and `0x801C029C` to X and Z,
respectively, again rounding toward zero. Each result is stored as a
halfword. For example, [sub_80050768](../scr/st/sub_80050768.c) adds cosine
and sine
results divided by 32 to the first pair on one path, while
[sub_8004E324](../scr/st/sub_8004E324.c) halves the second pair on one path
and clears sufficiently small results. **OPEN:** the complete set of
writers and the meanings of these values have not been established.

After this processing, if the signed 16-bit movement magnitude is nonzero and
player byte `0x21E` is zero, `ST_PlayerUpdateMovement` calls
[ST_ApplyDirectionalMovementWithCollision](../scr/st/ST_ApplyDirectionalMovementWithCollision.c)
with the computed angle and magnitude. That function calculates X and Z
components using cosine and sine, uses `ST_PLAYER_VERTICAL_MOVE` for Y, and
queries collisions. It can add that displacement to position before the
state machine calls `ST_PlayerMoveWithCollision`. Its collision-result branches and retry counter are described below.

## Turning and pitch input

The [default bindings](../scr/st/ST_InitDefaultControlBindings.c) store
`0x80` and `0x20` at control offsets `0x26` and `0x28`; the held button mask
is at `0x801CD182`. [ST_PlayerUpdateTurning](../scr/st/ST_PlayerUpdateTurning.c)
tests the first binding before the second, so the first takes precedence if
both are held. The corresponding signed 16-bit turn rate is at player offset
`0x2CA`. It changes by the signed 32-bit turn limit at `0x2E8` shifted right
by two bits. For a positive limit and values that do not wrap in 16 bits, the
first binding increases the rate up to the limit, the second decreases it to
the negative limit, and neither moves it toward zero by the same step. The
rate is stored as a halfword before the signed clamp.

The routine adds that rate to the halfword at player offset `0x27A` and masks
the result with `0xFFF`. The [state machine](../scr/st/sub_80042D74.c) sets
the turn limit to `0x20`, or `0x28` when neither binding at control offsets
`0x22` and `0x24` is held. Other state conditions can modify this value.
For limit 40 and no halfword overflow, holding the first turn binding from
zero produces rates 10, 20, 30, 40 and 40 on successive calls; releasing it
produces 30, 20, 10 and 0.

The same turning function reads bindings at control offsets `0x2E` and
`0x30`, with default masks `0x100` and `0x200`. With exactly one held, it
changes the signed halfword at player offset `0x2CC` by -3 or +3 and limits
it to -32 or +32 respectively, after storing the arithmetic result as a
halfword. With neither held, it moves that value toward zero by 3, with the
same halfword conversion. With both held, let `p` be the signed halfword at
player offset `0x278` and `r` the halfword at `0x2CC`:

- If `p > 0x800`, compute `target = (0x1080 - p) >> 2`, add 8 to `r` and
  truncate to a signed halfword. If `r > target`, replace it with `target`,
  or with `0x1080 - (uint16_t)p` if `target` is zero.
- If `p < 0x800`, compute `target = -((p - 0x80) >> 2)`, subtract 8 from
  `r` and truncate to a signed halfword. If `r < target`, replace it with
  `target`, or with `0x80 - (uint16_t)p` if `target` is zero.
- If `p == 0x800`, leave `r` unchanged.

If the resulting signed `r` is nonzero, the routine adds it to the pitch
halfword and masks the result by `0xFFF`. For positive `r`, it passes the
new pitch as the first argument and `0x2BC` as the second argument to
[sub_8004A194](../scr/st/sub_8004A194.c); if the helper returns nonzero, it
sets pitch to `0x2BC`. For negative `r`, it passes `0xD44` first and the new
pitch second; a nonzero return sets pitch to `0xD44`. The helper returns
whether `((first + 0x800) & 0xFFF)` is greater than
`((second + 0x800) & 0xFFF)`. A zero rate does not update pitch.

The port also writes the halfwords at `0x801991A0` and `0x801991A2` directly
when relative mouse input is enabled, using a sensitivity of 1.5 and clamping
its mouse pitch accumulator to -800..800. F1 toggles mouse capture; see
[PsyX_pad.cpp](../PsyCross/src/pad/PsyX_pad.cpp). These addresses correspond
to player offsets `0x278` and `0x27A`.

## Collision query and response

When the query path is entered, `ST_PlayerMoveWithCollision` builds a shared query at `0x801B5948` from the
player position and proposed X/Y/Z displacement. It supplies `0x100` and
`0x640` as query dimensions or offsets and enables checks with flags `0x432`.
If movement magnitude at `0x2C8` is nonzero and player byte `0x21E` is zero,
the routine skips this query path and continues at the later vertical update.
**OPEN:** the exact collision shape and the meaning of both dimensions have
not been established. [ST_RunCollisionQuery](../scr/st/ST_RunCollisionQuery.c) dispatches the enabled checks; among
them, [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c) searches level geometry. The query returns a bitmask and
stores additional collision data in the shared query structure.

### Query dispatcher: observed input and output flags

[ST_RunCollisionQuery](../scr/st/ST_RunCollisionQuery.c) reads a 32-bit
input mask at query offset `0`. It initializes the result to zero and handles
the following input bits in order:

| Input bit | Observed operation | Result contribution |
| --- | --- | --- |
| `0x2` | Calls [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c). | Uses the helper's return value as the initial result. |
| `0x10` | Calls [sub_8004BB88](../scr/st/sub_8004BB88.c), passing `2` as its second argument if input bit `0x400` is set, otherwise `0`. | ORs `0x10` if the helper returns anything other than `-1`. |
| `0x40` | Calls [sub_8004BE80](../scr/st/sub_8004BE80.c). | ORs `0x10` if the helper returns anything other than `-1`. |
| `0x20` | Calls [ST_QueryIndexedCollisionRecords](../scr/st/ST_QueryIndexedCollisionRecords.c). | ORs `0x40` if the helper returns anything other than `-1`. |
| `0x80` | Calls [sub_8003BEB4](../scr/st/sub_8003BEB4.c). If input bit `0x800` is also set, first adds `0x200` to the query halfword at `0x1C`. | ORs `0x80` if the helper returns anything other than `-1`. |

The three helpers for input bits `0x10`, `0x40`, and `0x20` have their
return values stored at `0x801B59A4`, `0x801B59A8`, and `0x801B59AC`,
respectively; if the corresponding input bit is clear, the dispatcher stores
`-1` at that address. After the geometry helper, the dispatcher adds the
unsigned query halfword at `0x1E` to its position word at `0x08` for the
remaining checks. Before returning it subtracts the then-current unsigned
halfword at `0x1E`, stores the final mask at `0x801B59BC`, and returns it.
For the movement routines' `0x432` input mask, the enabled checks are
`0x2`, `0x10`, and `0x20`, and `0x400` selects second argument `2` for
`sub_8004BB88`. With that second argument, the latter adds `0x200` to the
sum of query halfword `0x1C` and a candidate's halfword `0x2E` before its
horizontal comparison. The candidate records begin at `0x801BA138`, have a
stride of 212 bytes, and the scan examines at most 64 records. These
operations identify the result bits' producers; the
physical meanings of the helpers and their results are **OPEN**.

When the result is zero, the current candidate displacement is applied to
position. Bit `0x1` also applies it, after which other bits are still handled.
Bits `0x2` and `0x8` select separate horizontal correction paths. These use
query data and the original movement direction to form another X/Z candidate,
which is queried again. The routine calculates angles and perpendicular
directions; the precise meaning of the result bits and query fields is not
fully established. The routine tracks each of
these two correction cases and applies the current candidate if that same
case is encountered again.

## Directional collision pass

The following observations come from
[ST_ApplyDirectionalMovementWithCollision](../scr/st/ST_ApplyDirectionalMovementWithCollision.c).
Its arguments are the angle and magnitude computed by
`ST_PlayerUpdateMovement`. It starts with `x = rcos(angle) * magnitude / 4096`
and `z = rsin(angle) * magnitude / 4096`, using the low 32 bits of each
product and rounding the signed division toward zero. It copies the signed
vertical-movement halfword into `y`. It initializes the query at
`0x801B5948` from the player position, with flags `0x432` and halfwords
`0x100` and `0x640`. Before each query, it writes the low halfwords of
`x + player[0x1CC]`, `y`, and `z + player[0x1D0]` into the query vector.
It also writes an angle from `ratan2` of the first and third vector components
and the low halfword of `SquareRoot0(x*x + z*z)`. Products and additions here
use their low 32 bits.

The query result controls the next step, in this order:

- A zero result adds the current full-width `x`, `y`, and `z` to player
  position and finishes. A result equal to `1` does the same, provided fewer
  than three previous nonzero results have been processed.
- For any other nonzero result, the function exits without that position
  addition if three nonzero results have already been processed. Otherwise,
  it increments its counter. Thus the loop can issue a fourth query; a zero
  fourth result is still applied.
- If bit `0x2` is set, it calculates replacement X and Z from query word
  `0x3C` and signed halfwords `0x8C` and `0x90`: each component is the low
  32-bit product of `257 - word` and the corresponding halfword, divided by
  4096 with rounding toward zero. If this bit was handled on an earlier
  iteration, the replacement displacement is applied immediately.
- Otherwise, if bit `0x8` is set, the same operation uses query word `0x44`
  and signed halfwords `0x9C` and `0xA0`. A second occurrence also applies
  the replacement displacement immediately.
- Otherwise, if either bit in `0x50` is set, the function calls
  `ST_SetCollisionReference`, forms the X/Z difference between player position and query
  words `0xAC`/`0xB4`, and passes that vector to `VectorNormalS`. It derives
  replacement X/Z from that result and query values `0xBC` and `0x44C0`,
  with an additional `0x300` if bit `0x10` is set, or `0x100` otherwise.
  On a second occurrence it exits without adding the replacement to position.
- If none of these cases applies, it exits without adding the displacement.

On the first occurrence of any of the three correction cases, the function
also calculates an angle, uses `rcos`/`rsin` to add another X/Z component,
and queries again. On exit it adds one quarter of signed query halfwords
`0x47CE` and `0x47D2` to player movement halfwords `0x1CC` and `0x1D0`,
respectively, rounding toward zero, and stores the final X/Y/Z displacement
as halfwords at player offsets `0x260`, `0x262`, and `0x264`. The query fields
and result bits are identified by their observed offsets and masks. **OPEN:**
their complete geometric meanings have not been established.

### Collision reference selection

[ST_SetCollisionReference](../scr/st/ST_SetCollisionReference.c) takes a
pointer to a position and selects a reference in this order. Offsets below are
relative to the shared query at `0x801B5948`:

1. If result word `0x74` has bit `0x80`, select the four words at player
   offsets `0x248` through `0x254`, and write `0x100` and `0x3E8` to query
   halfwords `0x44C0` and `0x44C2`.
2. Otherwise, if query word `0x5C` is not `-1`, use it as an index into
   records starting at `0x801BA138` with stride 212 bytes. Select four words
   at record offset `0x74` and copy record halfwords `0x2E` and `0x30` to
   query offsets `0x44C0` and `0x44C2`.
3. Otherwise, if query word `0x64` is not `-1`, use it as an index into
   records starting at `0x801C2230` with stride 88 bytes. Select four words
   at record offset `0x24`. The record's unsigned halfword `0x0A` indexes
   24-byte records at `0x801C0310`. Copy the indexed record's halfwords
   `0x04` and `0x08` to query `0x44C0` and `0x44C2`; if bit `0x10` of the
   first record's byte `0x04` is set, multiply the first halfword by its byte
   `0x48`, take the low 32-bit product, and arithmetic-shift it right by 7
   before storing.
4. If both index words equal `-1` and bit `0x80` is clear, leave those query
   fields unchanged.

For each selected reference, the helper copies the four words to query
offsets `0xAC` through `0xB8`. It subtracts the selected X and Z words from
the supplied position's X and Z words, sums the low 32-bit squares with
32-bit wrap, passes the sum to `SquareRoot0`, and stores its result at query
offset `0xBC`. The dispatcher writes words `0x5C` and `0x64` from the
results of `sub_8004BB88` and `ST_QueryIndexedCollisionRecords`, respectively. **OPEN:** the
record types and the physical meanings of the two halfwords remain
unidentified.

### Indexed-record collision check

[ST_QueryIndexedCollisionRecords](../scr/st/ST_QueryIndexedCollisionRecords.c)
scans up to 240 records beginning at `0x801C2230`, with a stride of 88 bytes.
For each record it reads an unsigned index at offset `0x0A`. It skips the
record if the index is `0xFFFF`, if the record address equals the word at
`0x801C74C0`, or if halfword `0x04` of the indexed 24-byte record at
`0x801C0310` is zero.

The routine subtracts query X and Z (words `0x04` and `0x0C`) from record X
and Z (words `0x24` and `0x2C`) using 32-bit arithmetic. It skips the
record unless both differences lie in `[-0x2800, 0x2800]`. It calculates a
distance with `SquareRoot0` from the 32-bit wrapped sum of their squares,
and an angle with `ratan2` from those differences. It interprets the low
halfword of that angle as signed. From the distance it subtracts the low
32-bit product of query signed halfword `0x22` and
`rcos(query[0x20] - angle)`, divided by 4096
with rounding toward zero. It skips the record unless this result is less
than the sum of unsigned query halfword `0x1C` and the indexed record's
unsigned halfword `0x04`.

After a record passes that horizontal test, the function compares its Y
word at `0x28` to query Y at `0x08`. If record Y is numerically less, it returns `-1`
immediately when record Y is less than query Y minus unsigned query
halfword `0x1E`. Otherwise, it returns `-1` immediately when query Y is
less than record Y minus the record's unsigned halfword `0x12`. If neither
rejection applies, it returns the current record index. If no record passes
the earlier checks, it returns `-1`. The comparisons are signed 32-bit
comparisons after the indicated 32-bit subtraction. The early `-1` returns
do not continue scanning later records. **OPEN:** the record types and the
physical interpretation of their dimensions have not been established.

For this check in the player movement paths, query halfwords `0x1C` and
`0x1E` are 256 and 1600. The dispatcher adds 1600 to query Y before calling
the check. If the initial query Y is the player Y and the additions and
subtractions do not wrap, the vertical comparisons accept a record Y in the
inclusive interval from player Y to `player Y + 1600 + record[0x12]`.
The horizontal comparison adds 256 to the indexed record's `0x04` value.
These are properties of this particular check; they do not establish the
player's physical height or all checks made by the dispatcher.

## Vertical movement

In [ST_PlayerMoveWithCollision](../scr/st/ST_PlayerMoveWithCollision.c),
`ST_PLAYER_VERTICAL_MOVE` (`0x2D4`) is a signed 16-bit value used as the Y
component of collision-tested movement. A positive value increases the stored
Y coordinate when that displacement is applied. On the path that applies the
candidate, the current value is used before the later vertical update. After the
collision query, let `q` be its word at `0x801B5974` and `d = q - 0x640`
(1600). When the geometry search finds no candidates, `ST_QueryLevelGeometry` sets q
to `0x80000`. With a zero vertical value and this positive difference, the
update writes 16; with a positive value and the relevant query flags clear,
it adds 48. The initial
player Y is also initialized from a decimal text field shifted by 12 bits,
less `0x640`, while the collision query
temporarily adds that same offset to Y. These operations alone do not prove
which physical direction is up. **OPEN:** the exact meaning of q and the offset's relationship to
the player's full height are not established. The branches below describe
stored values.

For `d < 0`:

- If vertical movement is positive, call `sub_80057B48`, then arithmetic-shift
  the resulting signed value right by 2.
- If it is zero and `-31 <= d < 0`, set it to `-d`; if `d <= -32`, set it
  to -16. In the latter case clear the movement limit when `d >= -1024`.
- If it is negative, subtract 48 and store the result in a signed halfword.
  If `q - stored_result`, wrapped to 32 bits and interpreted as signed, is at
  least 1601, the
  code replaces that halfword with the low 16 bits of `q - 1600`. Clear the
  movement limit when `d >= -1024`.

For example, with d fixed at -100, starting at zero gives -16, -64, -100
on successive updates. Actual query results may change as the player moves.

For `d > 0`:

- A negative vertical value is divided by two, rounding toward zero.
- A zero value becomes `d` if `d < 32`, otherwise 16; when `32 <= d <=
  1024`, the movement limit is also cleared.
- A positive value increases by 48 only if the collision query flags above
  the low four bits are zero. The movement limit is cleared if `d <= 1024`.

When `d == 0`, vertical movement and the byte at player offset `0x239` are
cleared. Other collision branches may change the applied displacement. These
are per-update stored integer values; the update frequency and physical units
have not been established. With one spatial cell as one world unit, divide
positions and movement displacements by 4096 to obtain values with 12
fractional bits. See [STAGE_UNITS.md](STAGE_UNITS.md) for the extracted
geometry and the relationship between model, grid, and player coordinates.

Near the end of `ST_PlayerMoveWithCollision`,
[ST_UpdatePlayerCollisionYFlags](../scr/st/ST_UpdatePlayerCollisionYFlags.c)
compares query words `0x4C` and `0x58` with the player's Y word at `0x24C`,
using signed 32-bit comparisons. For each query word other than the sentinel
`0x80000`, it writes 1 to the corresponding player word at `0x2EC` or
`0x2F0` if the query value is less than player Y, or 0 otherwise. A sentinel
leaves that flag unchanged. If byte `0x801B5924` is 10 and signed player
halfword `0x178` is `0x147`, the function clears `0x2F0` even if the second
query word is the sentinel. The game state machine reads `0x2F0` when
choosing subsequent behavior. **OPEN:** the physical meanings of these flags
and the special-case values have not been established.
