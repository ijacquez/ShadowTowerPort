# Player movement

The [game state machine](../scr/st/ST_UpdatePlayerState.c) calls
`ST_PlayerUpdateTurning`, `ST_PlayerUpdateMovement`, and
`ST_PlayerMoveWithCollision` in that order in several states. The code is
recompiled MIPS: `ctx` registers and `MEM_*`
accesses represent emulated state, and comments retain original instructions.
`ST_PlayerState.h` names confirmed offsets from the player-state base
`0x80198F28`; it does not define a native C struct.
[ST_PlayerControls.h](../scr/st/ST_PlayerControls.h) names the verified
movement and view binding offsets in the control state at `0x801CD180`.
The counter names describe their demonstrated effects on movement limits;
their in-game meanings remain **OPEN**. Some collision fields and recompiled
functions still have offsets or generated names because their complete roles
are not established.

## Update timing in the port

[ST_MainGameLoop](../scr/st/ST_MainGameLoop.c) calls `ST_UpdatePlayerState`
once per loop iteration, then calls `DrawScene`. The scene calls
[ST_PresentScene](../scr/st/ST_PresentScene.c), which calls
[ST_SyncAndSetDrawEnvironments](../scr/st/ST_SyncAndSetDrawEnvironments.c);
that routine calls [VSync](../scr/st/VSync.c). The port's `VSync` targets
an interval of 1/24 second between calls, using millisecond-truncated sleep
when more than one millisecond remains. Other work and other calls to `VSync`
can affect the elapsed time between movement updates. The target is not a
guaranteed update rate, and the original game's update rate is **OPEN**.

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
step. The gameplay state normally sets L to 192 (`0xC0`, or 0.046875 spatial
cells), but can reduce or clear it.
For example, starting from zero with only positive Z held and L unchanged,
Z is 48 (`0x30`, or 0.01171875 cells), 96 (`0x60`, 0.0234375),
144 (`0x90`, 0.03515625), 192 (`0xC0`, 0.046875), then 192 on successive
updates. On release from 192, it decreases by 24 (`0x18`, 0.005859375) per
update. The shifts truncate the step; for example,
`L >> 3` is zero when `L` is between 1 and 7. The default and modified
limits are set in [ST_UpdatePlayerState.c](../scr/st/ST_UpdatePlayerState.c).

On the state machine path beginning at `L_8004323C`, `ST_PLAYER_MOVE_LIMIT` is
set to `0xC0` (0.046875 cells) and `ST_PLAYER_TURN_LIMIT` to 32. If neither
`ST_CONTROL_MOVE_Z_POSITIVE` nor `ST_CONTROL_MOVE_Z_NEGATIVE` is held, the turn
limit becomes 40. A nonzero signed `ST_PLAYER_MOVE_REDUCTION_COUNTER` (`0x19C`)
changes the movement limit to `0x60` (0.0234375 cells). A nonzero signed
`ST_PLAYER_MOVE_SUPPRESSION_COUNTER` (`0x194`) then clears the movement limit; if
`ST_PLAYER_EFFECT_FLAGS` (`0x2C`) also has bit `0x10` set, it clears the turn limit.

Finally, if the unsigned `ST_PLAYER_MOVE_PENALTY_TOTAL` (`0x22`) exceeds
`ST_PLAYER_MOVE_PENALTY_THRESHOLD` (`0x24`), the code subtracts their difference divided by 5, discarding the
remainder, from the movement limit and clamps a negative result to zero.
These changes precede the subsequent player-state dispatch; other paths can
change the limits again. **OPEN:** the in-game meanings of these counters and
the penalty inputs have not been established.

[ST_UpdatePlayerCountersAndEffects](../scr/st/ST_UpdatePlayerCountersAndEffects.c) processes
`ST_PLAYER_MOVE_SUPPRESSION_COUNTER`/`ST_PLAYER_MOVE_SUPPRESSION_TARGET`
(`0x194`/`0x196`) and `ST_PLAYER_MOVE_REDUCTION_COUNTER`/`ST_PLAYER_MOVE_REDUCTION_TARGET`
(`0x19C`/`0x19E`) as current/target pairs through
[ST_UpdateCounterAndTarget](../scr/st/ST_UpdateCounterAndTarget.c). When
both signed current and target are zero, the helper returns `-1`. Otherwise,
if current is less than target it increments current; if current is at least
target it clears target and decrements current. The calculated current is
stored as a halfword, so it wraps at the signed halfword boundaries. The
helper writes its calculated values only when its fifth argument is nonzero.
For these two pairs it returns `current * 4096 / 20`, truncated toward zero,
when current is below 20, or 4096 otherwise. The state machine calls
`ST_UpdatePlayerCountersAndEffects` with the write-enable argument set to 1 before applying the
movement-limit rules above. **OPEN:** the game meanings and all producers of
these counters and targets have not been established.

[ST_ResetPlayerCountersAndState](../scr/st/ST_ResetPlayerCountersAndState.c)
clears all player halfwords from `0x190` through `0x1C2`, including both
movement-related counter pairs, and clears player word `0x2C`. It then calls
[ST_ResetPlayerViewAndVerticalMovement](../scr/st/ST_ResetPlayerViewAndVerticalMovement.c),
which clears vertical movement and the view adjustments described below.

When the helper's signed halfword result for the suppression pair is
not `-1`, `ST_UpdatePlayerCountersAndEffects` clears the movement and turn
limits via `ST_ClearPlayerMovementLimits`, and clears player movement
halfwords `ST_PLAYER_MOVE_X`, `ST_PLAYER_MOVE_Y`, and `ST_PLAYER_MOVE_Z`.
It also clears all three halfwords in
each contribution set at `0x801C74B4` and `0x801C0298`, as well as the
three halfwords at `0x801BA116`. If the word containing the counter and
target is then zero, it clears bits `0x08` and `0x10` in player word `0x2C`.

[sub_8005A3A8](../scr/st/sub_8005A3A8.c) sets `ST_PLAYER_MOVE_PENALTY_TOTAL`
from a halfword calculated by
[sub_80059DC0](../scr/st/sub_80059DC0.c). The latter starts the halfword at
zero and iterates player halfword slots beginning at `0x38`. For slot indices
0 through 16, excluding 4, 5, 6, 11, 12, and 15, a slot other than `0xFFFF`
indexes a 44-byte record at `0x801C7810`; the unsigned record halfword at
`0x24` is added to the total with halfword wrapping. `sub_8005A3A8` sets the
`ST_PLAYER_MOVE_PENALTY_THRESHOLD` to `120 + ST_PLAYER_MAX_HP / 12`, using
unsigned HP and integer division. **OPEN:** the in-game names of the slot
records and their summed field have not been established.

[sub_80044674](../scr/st/sub_80044674.c) loads entry `0x1E0` from archive
index 1 and copies its eighth length-prefixed section to `0x801C7810`. The
same initialization associates archive index 1 with `COM/FDAT.T`. In the
supplied archive, that section's payload begins at file offset `0x1397290`
and is 13,824 bytes long. The 44-byte records used for the penalty total
are indexed within this copied data. **OPEN:** the full record format and
the in-game meaning of its halfword at `0x24` remain unestablished.

On paths in [sub_8003DBE0](../scr/st/sub_8003DBE0.c) that reach its
third-argument dispatch, values `0x08`, `0x10`, and `0x18` select the
suppression pair. The code sets `ST_PLAYER_MOVE_SUPPRESSION_TARGET` to 75. Depending
on bits `0x08` and `0x10` of player word `0x2C`, it either clears the
current counter and sets one of those bits, or sets the current counter to
20. Dispatch value `0x80` selects the reduction pair and sets its
target to 300. If bit `0x80` of player word `0x2C` is clear, the code clears
the current counter and sets the bit; otherwise, it sets the current counter
to 20. After this dispatch, the routine calculates limits using player fields
`0x20` and `0x1C8`; if a target interpreted as signed exceeds its calculated
limit, the routine replaces it with the low halfword of that limit. The
75 and 300 values above therefore are subject to this later cap. **OPEN:** the
in-game meanings of the two fields used to calculate the caps are not established.

[ST_SerializeGameState](../scr/st/ST_SerializeGameState.c) copies both
current/target pairs to buffer offsets `0x1880` through `0x188A`.
[ST_DeserializeGameState](../scr/st/ST_DeserializeGameState.c) restores the
same halfwords to the player. In [sub_80054A54](../scr/st/sub_80054A54.c),
dispatch value `0x11F` checks bits `0x18` of `ST_PLAYER_EFFECT_FLAGS`; when
either is set, it clears the suppression current and target and those bits.
These paths do not establish the effects' in-game names.

The routine processes the X/Z magnitudes and rotates the local vector by
negative player yaw using the GTE. It also damps existing movement components
and adds other contributions. The accumulators therefore are not directly
world-position increments.

More precisely, the first GTE command loads the accumulator words into IR1
and IR3, squares their signed low halfwords, and writes the results to the
GTE MAC registers. The routine passes the 32-bit sum of those two results to
`SquareRoot0` and stores the low halfword of its result in
`ST_PLAYER_MOVE_MAGNITUDE` (`0x2C8`). If that halfword interpreted as signed is zero, it sets the two
horizontal vector components to zero. Otherwise, it gives each square the
sign of its original 32-bit accumulator and divides by that signed halfword.
See [ST_PlayerUpdateMovement.c](../scr/st/ST_PlayerUpdateMovement.c), the
[GTE SQR implementation](../PsyCross/src/gte/PsyX_GTE.cpp), and the
[IR register definitions](../PsyCross/include/psx/gtereg.h). The integer
divisions discard any fractional remainder.

The rotation input's Y halfword is loaded from stack offset `0x1C`, which
the movement function does not write before loading it. **OPEN:** its source
and role are not established. In the supplied executable, the zero-angle
table entry read by [RotMatrix](../scr/st/RotMatrix.c) has a signed low
halfword of zero and a high halfword of `0x1000` (1.0).

Movement passes rotation angles `(0, -yaw, 0)` to `RotMatrix`. With the
above zero-angle entry, its X/Z rows have zero in their Y-input column for
every yaw. The [GTE MVMVA implementation](../PsyCross/src/gte/PsyX_GTE.cpp)
multiplies those matrix entries by the vector's Y halfword, so that halfword
contributes zero to rotated X and Z. The subsequent angle and magnitude use
those X/Z results. This conclusion depends on that executable table entry
still containing the stated value when the function runs. The same matrix's
Y row is `(0, 0x1000, 0)` (0.0, 1.0, 0.0), so the rotated Y is the signed
input halfword. The routine writes that result as a word to stack offset
`0x1C`, then squares it with the GTE and writes the square to stack offset
`0x2C`. The GTE's Y result and flags can therefore depend on the input even
though the horizontal results do not. **OPEN:** the final prior writer and
any later uses of this GTE state have not been established.

The rotation uses `-yaw` and writes GTE results back to the stack before
computing another angle and magnitude. The later code updates each signed
movement halfword `ST_PLAYER_MOVE_X`, `ST_PLAYER_MOVE_Y`, and
`ST_PLAYER_MOVE_Z` by subtracting one quarter of its value, rounded toward zero. It
then clears a resulting halfword in the range -3 through 3. It adds one
quarter of the signed halfword at `0x801C74B4` to X and one quarter of the
signed halfword at `0x801C74B8` to Z, rounding toward zero. It also adds half
the signed halfwords at `0x801C0298` and `0x801C029C` to X and Z,
respectively, again rounding toward zero. Each result is stored as a
halfword. [sub_80050768](../scr/st/sub_80050768.c) adds cosine and sine
results divided by 32 to the first pair on one path. At the end of its
240-record loop, it halves the signed halfwords at `0x801C74B4`,
`0x801C74B6`, and `0x801C74B8`, rounding toward zero, and clears each
result in the range -3 through 3. On a path following its 64-record loop,
[sub_8004E324](../scr/st/sub_8004E324.c) halves the signed halfwords at
`0x801C0298`, `0x801C029A`, and `0x801C029C` with an arithmetic right
shift, then clears each result in the range -3 through 3. Thus negative odd
values round down in the second set, unlike the first set. **OPEN:** the
complete set of writers and the meanings of these values have not been
established.

After this processing, if the signed 16-bit movement magnitude is nonzero and
`ST_PLAYER_DIRECTIONAL_MOVE_DISABLED` (`0x21E`) is zero, `ST_PlayerUpdateMovement` calls
[ST_ApplyDirectionalMovementWithCollision](../scr/st/ST_ApplyDirectionalMovementWithCollision.c)
with the computed angle and magnitude. That function calculates X and Z
components using cosine and sine, uses `ST_PLAYER_VERTICAL_MOVE` for Y, and
queries collisions. It can add that displacement to position before the
state machine calls `ST_PlayerMoveWithCollision`. Its collision-result branches and retry counter are described below.

[ST_ClearPlayerMovementLimits](../scr/st/ST_ClearPlayerMovementLimits.c)
clears the movement and turn limits, turn and pitch rates, and movement
magnitude, then sets `ST_PLAYER_DIRECTIONAL_MOVE_DISABLED` to 1. It does not clear the local
X/Z input accumulators. The [game state machine](../scr/st/ST_UpdatePlayerState.c)
and other game routines call it on different paths.

[ST_ClearPlayerMovementLimitsAndVertical](../scr/st/ST_ClearPlayerMovementLimitsAndVertical.c)
also clears vertical movement and player bytes `0x218` and `0x239`, clears
word `0x23C`, and sets byte `0x21C` to 1. It calls
`ST_ClearPlayerMovementLimits` and likewise leaves the local X/Z input
accumulators unchanged.

## Turning and pitch input

The [default bindings](../scr/st/ST_InitDefaultControlBindings.c) store
`0x80` and `0x20` at control offsets `0x26` and `0x28`; the held button mask
is at `0x801CD182`. [ST_PlayerUpdateTurning](../scr/st/ST_PlayerUpdateTurning.c)
tests the first binding before the second, so the first takes precedence if
both are held. The corresponding signed 16-bit `ST_PLAYER_TURN_RATE` (`0x2CA`)
changes by the signed 32-bit `ST_PLAYER_TURN_LIMIT` (`0x2E8`) shifted right
by two bits. For a positive limit and values that do not wrap in 16 bits, the
first binding increases the rate up to the limit, the second decreases it to
the negative limit, and neither moves it toward zero by the same step. The
rate is stored as a halfword before the signed clamp.

The routine adds that rate to `ST_PLAYER_YAW` (`0x27A`) and masks
the result with `0xFFF`. The [state machine](../scr/st/ST_UpdatePlayerState.c) sets
the turn limit to `0x20`, or `0x28` when neither binding at control offsets
`0x22` and `0x24` is held. Other state conditions can modify this value.
For limit 40 and no halfword overflow, holding the first turn binding from
zero produces rates 10, 20, 30, 40 and 40 on successive calls; releasing it
produces 30, 20, 10 and 0.

The same turning function reads bindings at control offsets `0x2E` and
`0x30`, with default masks `0x100` and `0x200`. With exactly one held, it
changes the signed `ST_PLAYER_PITCH_RATE` (`0x2CC`) by -3 or +3 and limits
it to -32 or +32 respectively, after storing the arithmetic result as a
halfword. With neither held, it moves that value toward zero by 3, with the
same halfword conversion. With both held, let `p` be the signed halfword at
`ST_PLAYER_PITCH` (`0x278`) and `r` the `ST_PLAYER_PITCH_RATE`:

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
[ST_IsShiftedAngleGreater](../scr/st/ST_IsShiftedAngleGreater.c); if the helper returns nonzero, it
sets pitch to `0x2BC`. For negative `r`, it passes `0xD44` first and the new
pitch second; a nonzero return sets pitch to `0xD44`. The helper returns
whether `((first + 0x800) & 0xFFF)` is greater than
`((second + 0x800) & 0xFFF)`. A zero rate does not update pitch.

The port also writes the halfwords at `0x801991A0` and `0x801991A2` directly
when relative mouse input is enabled, using a sensitivity of 1.5 and clamping
its mouse pitch accumulator to -800..800. F1 toggles mouse capture; see
[PsyX_pad.cpp](../PsyCross/src/pad/PsyX_pad.cpp). These addresses correspond
to `ST_PLAYER_PITCH` and `ST_PLAYER_YAW`.

## Collision query and response

When the query path is entered, `ST_PlayerMoveWithCollision` builds a shared query at `0x801B5948` from the
player position and proposed X/Y/Z displacement. It supplies `0x100` (0.0625
cells) as `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER` (`0x1C`) and `0x640`
(0.390625 cells) as `ST_COLLISION_QUERY_VERTICAL_PARAMETER` (`0x1E`), and
enables checks with `ST_COLLISION_QUERY_INPUT_FLAGS` (`0x00`) set to `0x432`.
If `ST_PLAYER_MOVE_MAGNITUDE` is nonzero and `ST_PLAYER_DIRECTIONAL_MOVE_DISABLED` is zero,
the routine skips this query path and continues at the later vertical update.
**OPEN:** the exact collision shape and the meaning of both dimensions have
not been established. [ST_RunCollisionQuery](../scr/st/ST_RunCollisionQuery.c) dispatches the enabled checks; among
them, [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c) searches level geometry. The query returns a bitmask and
stores additional collision data in the shared query structure.

For queries made by `ST_PlayerMoveWithCollision`, `ST_COLLISION_QUERY_DIRECTION` (`0x20`) is
set to `9999`.
`ST_QueryLevelGeometry` uses that value to build a signed-halfword vector
from the candidate displacement. When X and Z are both zero, the vector's
X and Z are zero. Otherwise it computes `angle = ratan2(Z, X)` and adds
`rcos(angle) * 0x100 / 4096` to X and `rsin(angle) * 0x100 / 4096` to Z,
with the divisions rounded toward zero. Its Y is the candidate Y plus
`0x640`. Each component is stored as a halfword. These operations establish
how the two query values affect this vector; their complete geometric
interpretation remains **OPEN**.
`ST_ApplyDirectionalMovementWithCollision` instead stores the result of
`ratan2` on its combined Z and X movement halfwords in `ST_COLLISION_QUERY_DIRECTION`.
When that halfword is not `9999`, the geometry routine uses it for the
cosine and sine terms and uses the query's Y displacement without adding
`0x640` to that vector component.

### Query dispatcher input and output flags

[ST_RunCollisionQuery](../scr/st/ST_RunCollisionQuery.c) reads a 32-bit
input mask `ST_COLLISION_QUERY_INPUT_FLAGS`. It initializes the result to zero and handles
the following input bits in order:

| Input bit | Operation | Result contribution |
| --- | --- | --- |
| `0x2` | Calls [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c). | Uses the helper's return value as the initial result. |
| `0x10` | Calls [ST_QueryActiveCollisionRecords](../scr/st/ST_QueryActiveCollisionRecords.c), passing `2` as its second argument if input bit `0x400` is set, otherwise `0`. | ORs `0x10` if the helper returns anything other than `-1`. |
| `0x40` | Calls [sub_8004BE80](../scr/st/sub_8004BE80.c). | ORs `0x10` if the helper returns anything other than `-1`. |
| `0x20` | Calls [ST_QueryIndexedCollisionRecords](../scr/st/ST_QueryIndexedCollisionRecords.c). | ORs `0x40` if the helper returns anything other than `-1`. |
| `0x80` | Calls [sub_8003BEB4](../scr/st/sub_8003BEB4.c). If input bit `0x800` is also set, first adds `0x200` (0.125 cells) to `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER`. | ORs `0x80` if the helper returns anything other than `-1`. |

The three helpers for input bits `0x10`, `0x40`, and `0x20` have their
return values stored at `0x801B59A4`, `0x801B59A8`, and `0x801B59AC`,
respectively; if the corresponding input bit is clear, the dispatcher stores
`-1` at that address. After the geometry helper, the dispatcher adds the
unsigned `ST_COLLISION_QUERY_VERTICAL_PARAMETER` to `ST_COLLISION_QUERY_POSITION_Y` for the
remaining checks. Before returning it subtracts the then-current unsigned
`ST_COLLISION_QUERY_VERTICAL_PARAMETER`, stores the final mask at `0x801B59BC`, and returns it.
For the movement routines' `0x432` input mask, the enabled checks are
`0x2`, `0x10`, and `0x20`, and `0x400` selects second argument `2` for
`ST_QueryActiveCollisionRecords`. With that second argument, the latter adds `0x200` (0.125 cells) to the
sum of `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER` and a candidate's halfword `0x2E` before its
horizontal comparison. The candidate records begin at `0x801BA138`, have a
stride of 212 bytes, and the scan examines at most 64 records. These
operations identify the result bits' producers; the
physical meanings of the helpers and their results are **OPEN**.

### Active-record collision check for movement

[ST_QueryActiveCollisionRecords](../scr/st/ST_QueryActiveCollisionRecords.c)
examines up to 64 records at `0x801BA138`, each 212 bytes long. With the
movement query's second argument of 2, a record is considered only if
`ST_ACTIVE_COLLISION_RECORD_STATE` equals 1, its
`ST_ACTIVE_COLLISION_RECORD_FILTER_FLAGS` has no bit in common with the byte
at `0x801C0239`, and its address differs from the word at `0x801C0270`.
Its `ST_ACTIVE_COLLISION_RECORD_CLASS_INDEX` selects a 192-byte entry at
`0x801BD638`; the entry's unsigned halfword at `0x16` must be nonzero.

The check subtracts the query X and Z from the record's
`ST_ACTIVE_COLLISION_RECORD_POSITION_X` and `_Z`, using 32-bit arithmetic,
and skips the record unless both differences are in
`[-0x2800, 0x2800]` (-2.5 to 2.5 cells). It computes their wrapped squared
sum, passes that to `SquareRoot0`, and obtains an angle with `ratan2`. From
the distance it subtracts the signed low-32-bit product of
`ST_COLLISION_QUERY_HORIZONTAL_MAGNITUDE` and
`rcos(ST_COLLISION_QUERY_DIRECTION - angle)`, divided by `0x1000` (1.0 in
the trigonometric scale), rounding toward zero. The record passes this test
only if the result is less than `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER`
plus `ST_ACTIVE_COLLISION_RECORD_HORIZONTAL_EXTENT` plus `0x200` (0.125
cells).

After a horizontal match, the check compares the signed record Y with the
query Y already adjusted by the dispatcher. If record Y is less, it rejects
the record when it is below query Y minus
`ST_COLLISION_QUERY_VERTICAL_PARAMETER`. Otherwise it rejects the record when
query Y is below record Y minus `ST_ACTIVE_COLLISION_RECORD_VERTICAL_EXTENT`.
Subtractions wrap to 32 bits. In movement mode, either vertical rejection
returns `-1` immediately; a vertical match returns the current record index.
If no horizontal candidate is accepted, it returns `-1`. **OPEN:** the
in-game identity of the records and the full meaning of their extent fields
are not established.

### Level-geometry result bits

When the geometry search has candidates, `ST_QueryLevelGeometry` sets its
four low result bits using the following signed comparisons. Let `D` be the
three signed displacement halfwords and let `H` and `V` be the unsigned
horizontal and vertical query parameters. `N1` and `N2` are the signed
halfword vectors at `ST_COLLISION_QUERY_FIRST_NORMAL_X` and
`ST_COLLISION_QUERY_SECOND_NORMAL_X`. For each dot product, the code wraps
products and sums to 32 bits, interprets the result as signed, and divides
by `0x1000` (1.0 in the normal-vector scale), rounding toward zero.

| Geometry result bit | Condition |
| --- | --- |
| `ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD` (`0x1`) | `ST_COLLISION_QUERY_CURRENT_Y_RESULT < V + D.y` |
| `ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD` (`0x2`) | `ST_COLLISION_QUERY_FIRST_NORMAL_RESULT < H - dot(D, N1) / 4096` |
| `ST_COLLISION_RESULT_VERTICAL_THRESHOLD` (`0x4`) | `ST_COLLISION_QUERY_VERTICAL_THRESHOLD_RESULT < 0x100 - D.y` |
| `ST_COLLISION_RESULT_SECOND_NORMAL_THRESHOLD` (`0x8`) | `ST_COLLISION_QUERY_SECOND_NORMAL_RESULT < H - dot(D, N2) / 4096` |

The additions and subtractions in the comparisons also wrap to 32 bits.
The normal-result words and vectors are copied from the geometry helpers'
scratch values before the function returns. If the candidate search is empty,
the geometry function returns zero without these comparisons. **OPEN:** the
complete physical interpretation of these thresholds and result bits.

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

[ST_ApplyDirectionalMovementWithCollision](../scr/st/ST_ApplyDirectionalMovementWithCollision.c)
takes the angle and magnitude computed by
`ST_PlayerUpdateMovement`. It starts with `x = rcos(angle) * magnitude / 4096`
and `z = rsin(angle) * magnitude / 4096`, using the low 32 bits of each
product and rounding the signed division toward zero. It copies the signed
vertical-movement halfword into `y`. It initializes the query at
`0x801B5948` from the player position, with flags `0x432` and halfwords
`0x100` and `0x640`. Before each query, it writes the low halfwords of
`x + ST_PLAYER_MOVE_X`, `y`, and `z + ST_PLAYER_MOVE_Z` into the query vector.
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
- If `ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD` is set, it calculates replacement X and Z from
  `ST_COLLISION_QUERY_FIRST_NORMAL_RESULT` and signed
  `ST_COLLISION_QUERY_FIRST_NORMAL_X` and `ST_COLLISION_QUERY_FIRST_NORMAL_Z`: each component is the low
  32-bit product of `257 - word` and the corresponding halfword, divided by
  4096 with rounding toward zero. If this bit was handled on an earlier
  iteration, the replacement displacement is applied immediately.
- Otherwise, if `ST_COLLISION_RESULT_SECOND_NORMAL_THRESHOLD` is set, the same operation uses
  `ST_COLLISION_QUERY_SECOND_NORMAL_RESULT`, `ST_COLLISION_QUERY_SECOND_NORMAL_X`,
  and `ST_COLLISION_QUERY_SECOND_NORMAL_Z`. A second occurrence also applies
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
and queries again. On exit it adds one quarter of signed
`ST_COLLISION_QUERY_RECORD_VECTOR_SUM_X` (`0x47CE`) and
`ST_COLLISION_QUERY_RECORD_VECTOR_SUM_Z` (`0x47D2`) to `ST_PLAYER_MOVE_X` and `ST_PLAYER_MOVE_Z`,
respectively, rounding toward zero, and stores the final X/Y/Z displacement
as `ST_PLAYER_COLLISION_DISPLACEMENT_X`, `ST_PLAYER_COLLISION_DISPLACEMENT_Y`,
and `ST_PLAYER_COLLISION_DISPLACEMENT_Z` (`0x260`, `0x262`, `0x264`). The query fields
and result bits are identified by their offsets and masks. **OPEN:**
their complete geometric meanings have not been established.

Each movement query clears `ST_COLLISION_QUERY_RECORD_VECTOR_SUM_X`,
`ST_COLLISION_QUERY_RECORD_VECTOR_SUM_Y` (`0x47D0`), and
`ST_COLLISION_QUERY_RECORD_VECTOR_SUM_Z` before dispatch. On a qualifying
current-position collision-record path, the three- and four-vertex geometry
helpers form a vector from signed record bytes 0 and 1 as `(byte0, 0, byte1)`,
rotate it by the selected placement matrix with the GTE, and add the resulting
halfwords to these sums with halfword wrapping. They skip this vector update
when the record's flag halfword at offset 2 has bit `0x400` set. Other
geometric checks also determine whether this path is reached.
`ST_UpdatePlayerState` subsequently halves each sum, rounding toward zero,
and clears each halved result in the range -4 through 3 on that state path.
**OPEN:** the in-game meaning of the record vector and the complete set of
paths that modify the sums remain unestablished.

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
results of `ST_QueryActiveCollisionRecords` and `ST_QueryIndexedCollisionRecords`, respectively. **OPEN:** the
record types and the physical meanings of the two halfwords remain
unidentified.

### Indexed-record collision check

[ST_QueryIndexedCollisionRecords](../scr/st/ST_QueryIndexedCollisionRecords.c)
scans up to 240 records beginning at `0x801C2230`, with a stride of 88 bytes.
For each record it reads an unsigned index at offset `0x0A`. It skips the
record if the index is `0xFFFF`, if the record address equals the word at
`0x801C74C0`, or if halfword `0x04` of the indexed 24-byte record at
`0x801C0310` is zero.

The routine subtracts `ST_COLLISION_QUERY_POSITION_X` (`0x04`) and
`ST_COLLISION_QUERY_POSITION_Z` (`0x0C`) from record X
and Z (words `0x24` and `0x2C`) using 32-bit arithmetic. It skips the
record unless both differences lie in `[-0x2800, 0x2800]`. It calculates a
distance with `SquareRoot0` from the 32-bit wrapped sum of their squares,
and an angle with `ratan2` from those differences. It interprets the low
halfword of that angle as signed. From the distance it subtracts the low
32-bit product of signed `ST_COLLISION_QUERY_HORIZONTAL_MAGNITUDE` (`0x22`) and
`rcos(ST_COLLISION_QUERY_DIRECTION - angle)`, divided by 4096
with rounding toward zero. It skips the record unless this result is less
than the sum of unsigned `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER` and the indexed record's
unsigned halfword `0x04`.

After a record passes that horizontal test, the function compares its Y
word at `0x28` to `ST_COLLISION_QUERY_POSITION_Y` (`0x08`). If record Y is numerically less, it returns `-1`
immediately when record Y is less than query Y minus unsigned query
`ST_COLLISION_QUERY_VERTICAL_PARAMETER`. Otherwise, it returns `-1` immediately when query Y is
less than record Y minus the record's unsigned halfword `0x12`. If neither
rejection applies, it returns the current record index. If no record passes
the earlier checks, it returns `-1`. The comparisons are signed 32-bit
comparisons after the indicated 32-bit subtraction. The early `-1` returns
do not continue scanning later records. **OPEN:** the record types and the
physical interpretation of their dimensions have not been established.

For this check in the player movement paths, `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER`
is `0x100` (0.0625 cells) and `ST_COLLISION_QUERY_VERTICAL_PARAMETER` is
`0x640` (0.390625 cells). The dispatcher adds 1600 to query Y before calling
the check. If the initial query Y is the player Y and the additions and
subtractions do not wrap, the vertical comparisons accept a record Y in the
inclusive interval from player Y to `player Y + 1600 + record[0x12]`.
The horizontal comparison adds 256 to the indexed record's `0x04` value.
These are properties of this particular check; they do not establish the
player's physical height or all checks made by the dispatcher.

## Scene-view position

[ST_BuildPlayerView](../scr/st/ST_BuildPlayerView.c) copies
`ST_PLAYER_POSITION_X` and `ST_PLAYER_POSITION_Z` into the position supplied to scene
rendering. It calculates that position's Y as `ST_PLAYER_POSITION_Y` plus
`ST_PLAYER_VIEW_OSCILLATION_Y` and `ST_PLAYER_VIEW_Y_ADJUSTMENT_B`, minus
`ST_PLAYER_VIEW_Y_OFFSET` (`0x640`, 0.390625 cells), with 32-bit wrapping.
It copies `ST_PLAYER_VIEW_ROTATION_X`, `ST_PLAYER_VIEW_ROTATION_Y`, and
`ST_PLAYER_VIEW_ROTATION_Z` to the accompanying
rotation. [ST_MainGameLoop](../scr/st/ST_MainGameLoop.c) passes these outputs
to [DrawScene](../scr/st/DrawScene.c). Thus 1600 is also an offset in the
scene-view Y calculation; this does not establish the player's full height.

`ST_PLAYER_VIEW_OSCILLATION_Y` (`0x2CE`) is updated by
`ST_PlayerMoveWithCollision` when player byte `0x239` is zero. If
the byte at `ST_ADJUST_WALK_SETTING_ADDRESS` (`0x801CD122`) is zero, the adjustment is cleared. Otherwise the routine adds
the unsigned `ST_PLAYER_MOVE_MAGNITUDE` to the unsigned
`ST_PLAYER_VIEW_OSCILLATION_PHASE` (`0x2D0`), masks the sum with `0xFFF`, and stores it as the new phase.
It computes `t = rsin(phase) >> 5` using an arithmetic shift, then stores
`abs(t) - (abs(t) >> 2)` as the adjustment. If player byte `0x239` is
nonzero, this part of the routine leaves both phase and adjustment unchanged.
[ST_ResetPlayerViewAndVerticalMovement](../scr/st/ST_ResetPlayerViewAndVerticalMovement.c) clears the phase and both Y
adjustments. **OPEN:** the meanings of byte `0x239` and the second
adjustment have not been established.

[sub_8002B2E8](../scr/st/sub_8002B2E8.c) also reads `ST_ADJUST_WALK_SETTING_ADDRESS` as
the first of four consecutive bytes. For each byte it renders a pair of
text entries with their two styles exchanged when the byte equals 1. On the
supplied disc, the first setting is labeled **ADJUST WALK**, with ON and OFF
choices. Its movement effect is the view oscillation described above.

In [ST_UpdateLevelStreaming](../scr/st/ST_UpdateLevelStreaming.c), when
signed halfwords at `0x801B5920` and `0x801B5922` both equal 1, the game
queues a resource, clears player word `0x174`, sets player halfword `0x178`
to `-1`, and copies the level table. It then calls
[ST_TranslatePlayerPosition](../scr/st/ST_TranslatePlayerPosition.c) if the
signed byte at `0x801B5940` is not `0x7F`. This adds offsets directly to
player X, Y, and Z. Relative to `0x801B5920`, those offsets are
`(byte[0x20] << 12) + (byte[0x23] << 7)` for X,
`(byte[0x22] << 12) + (byte[0x24] << 7)` for Y, and
`(byte[0x21] << 12) + (byte[0x25] << 7)` for Z. Each byte is signed and
each position addition wraps to 32 bits. **OPEN:** the record's full meaning
and all callers that request these offsets have not been established.

On its request path, [ST_RequestLevelStreaming](../scr/st/ST_RequestLevelStreaming.c)
sets the streaming status to 1, the phase to 0, and copies six bytes from
the caller's stack arguments at offsets `0x14` through `0x28` in four-byte
steps to streaming-state offsets `0x20` through `0x25`. A separate path in
[ST_StartLevelStreamingAndWait](../scr/st/ST_StartLevelStreamingAndWait.c) sets status to 1 and phase to 0,
and writes `0x7F` to offsets `0x20`, `0x21`, and `0x22`; the first of those
bytes causes the later translation to be skipped.

On one path through [sub_80050768](../scr/st/sub_80050768.c), the six
arguments come from the current 88-byte record in the array beginning at
`0x801C2230`: record bytes `0x50`, `0x52`, and `0x51` provide the coarse
X, Z, and Y components, and bytes `0x4B`, `0x4C`, and `0x4F` provide the
corresponding fine components. The resulting translation uses steps of 4096
and 128 stored position units, respectively. These are the same 88-byte
records scanned by `ST_QueryIndexedCollisionRecords`.

## Vertical movement

In [ST_PlayerMoveWithCollision](../scr/st/ST_PlayerMoveWithCollision.c),
`ST_PLAYER_VERTICAL_MOVE` (`0x2D4`) is a signed 16-bit value used as the Y
component of collision-tested movement. A positive value increases the stored
Y coordinate when that displacement is applied. **Positive world Y is down;
negative world Y is up.** A positive vertical movement value therefore moves
the player down, and a negative value moves the player up. On the path that applies the
candidate, the current value is used before the later vertical update. After the
collision query, let `q` be `ST_COLLISION_QUERY_PROPOSED_Y_RESULT` (`0x2C`,
address `0x801B5974`) and `d = q - ST_PLAYER_COLLISION_Y_OFFSET`
(`0x640`, 0.390625 cells). When the geometry search finds no candidates, `ST_QueryLevelGeometry` sets q
to `0x80000`. With a zero vertical value and this positive difference, the
update writes 16; with a positive value and the relevant query flags clear,
it adds 48. These are downward changes. The initial
player Y is also initialized from a decimal text field shifted by 12 bits,
less `0x640`, while the collision query
temporarily adds that same offset to Y. **OPEN:** the physical meaning of q and the offset's relationship to
the player's full height are not established. The branches below describe
stored values.

In spatial-cell units, the fixed-point quantities used below have 12
fractional bits:

| Stored value | Spatial-cell value |
| --- | --- |
| `0x10` (16) | 0.00390625 |
| `0x20` (32) | 0.0078125 |
| `0x30` (48) | 0.01171875 |
| `0x100` (256) | 0.0625 |
| `0x240` (576) | 0.140625 |
| `0x3E8` (1000) | 0.244140625 |
| `0x400` (1024) | 0.25 |
| `0x480` (1152) | 0.28125 |
| `0x640` (1600) | 0.390625 |
| `0x800` (2048) | 0.5 |
| `0x1000` (4096) | 1.0 |

Before the vertical update, [ST_CheckHighDownwardMovement](../scr/st/ST_CheckHighDownwardMovement.c)
checks the current vertical movement and signed `ST_COLLISION_QUERY_CURRENT_Y_RESULT`
(`0x30`). If movement is at least `0x480` (0.28125 cells per update)
and that query word is at least `0x640` (0.390625 cells), it calls
[sub_8003BD14](../scr/st/sub_8003BD14.c) with zero and sets player byte
`0x238` to 1. Unless player byte `0x218` is already 4, that helper sets it
to 4, clears the current HP halfword at player offset `0x02`, and performs
additional state updates. The [player-stat display](../scr/st/sub_80029B74.c)
reads `ST_PLAYER_CURRENT_HP`, `ST_PLAYER_MAX_HP`, `ST_PLAYER_CURRENT_MP`,
and `ST_PLAYER_MAX_MP`. **OPEN:** the game meanings of
bytes `0x218` and `0x238` are not established.

The level-rendering path establishes the Y direction. For a level,
unrotated view, [ST_TransformLevelOriginsForView](../scr/st/ST_TransformLevelOriginsForView.c) subtracts player
Y and the two view adjustments from a level placement's Y. The rotation
matrix from [RotMatrix](../scr/st/RotMatrix.c) is the identity at zero
angles; [ST_BuildQuarterTurnMatrices](../scr/st/ST_BuildQuarterTurnMatrices.c) preserves the Y axis in
the placement matrices. [ST_ProjectLevelVertices](../scr/st/ST_ProjectLevelVertices.c) uses those
matrices and the placement translation to project level vertices. In the
[GTE projection](../PsyCross/src/gte/PsyX_GTE.cpp), for positive depth and
without saturation, increasing relative Y increases screen Y. The
[renderer](../PsyCross/src/render/PsyX_render.cpp) maps increasing screen Y
downward. Thus increasing a level vertex's world Y moves it down on screen,
and increasing player Y moves the view down relative to the level.

For a selected three- or four-vertex collision record, the geometry helpers
rotate the record's referenced vector into signed halfwords `N` and select
its first transformed vertex `V`. They calculate a value from the proposed
position `P` as `N dot P - N dot V`, with each product and sum wrapped to
32 bits, then divide the signed result by 4096 toward zero. On qualifying
paths, the smaller signed result replaces a scratch value initialized to
`0x80000`. A record reaching that update has bit `0x1` set in its halfword
at offset 2, a proposed-position result of at least `-256`, and three or
four nonnegative edge-test results after the helpers' 32-bit left shift by
12. Earlier checks can reject the record as well. If that
scratch value is still the sentinel after both helpers,
the geometry query copies another scratch value calculated from the current
position instead. It also copies the associated rotated vector.

When the resulting value is not the sentinel and its associated Y halfword
is nonzero, the geometry query calculates a horizontal magnitude using the
associated X halfword squared plus the product of its Z halfword and a Z
halfword from the other scratch result, with 32-bit arithmetic. If the
`SquareRoot0` result is nonzero, it obtains an angle from `ratan2(-Y,
horizontalMagnitude)` and divides the 32-bit result of `value << 12` by
`rsin(angle)`, with signed division toward zero when the divisor is nonzero.
The code contains a division-by-zero break for a zero divisor. When the
horizontal magnitude is zero, it leaves the value unchanged. It writes the
result to `q`.
See [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c),
[ST_TestThreeVertexCollisionRecords](../scr/st/ST_TestThreeVertexCollisionRecords.c),
and [ST_TestFourVertexCollisionRecords](../scr/st/ST_TestFourVertexCollisionRecords.c).

For the particular case where the selected proposed-position record has
rotated vector `(0, -0x1000, 0)` (0.0, -1.0, 0.0), and the intermediate arithmetic does not
wrap, `q` is the record's plane Y minus the proposed player Y. Its horizontal
magnitude is zero, so the later angle-based adjustment is skipped. On that
path, `q = 0x640` (0.390625 cells) places the proposed player position 1600 stored units above
the plane, and `d` is the excess or shortfall from that separation. For
example, the initial stage contains a plane 576 units below the initial
player position; if it supplies the selected result with zero proposed Y
movement, then `q = 0x240` (0.140625 cells) and `d = -0x400` (-0.25 cells).
This conditional calculation does
not establish which collision record wins or the player's full height. See
[STAGE_UNITS.md](STAGE_UNITS.md) for that plane's data.

For `d < 0`:

- If vertical movement is positive, call `ST_HandleDownwardCollisionFeedback`, then arithmetic-shift
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

For the positive-vertical branch, [ST_HandleDownwardCollisionFeedback](../scr/st/ST_HandleDownwardCollisionFeedback.c)
does not call either helper when its signed input is below `0x100` (0.0625 cells per update).
Otherwise it subtracts `0x100`, limits the result to `0x200` (0.125 cells per
update), and calls `sub_800582F0` with ID `0x14` and parameter
`0x40 + (limited_result >> 3)`. The latter can reach `SsUtKeyOn` through
`sub_80058318`. It also calls
[ST_SetControllerActuatorFeedback](../scr/st/ST_SetControllerActuatorFeedback.c)
with `limited_result >> 2`.

When `ST_CONTROLLER_FEEDBACK_SETTING_ADDRESS` is nonzero and that input is
positive, the actuator helper adds 64, caps the result at 255, stores it in
`ST_CONTROLLER_ACTUATOR_SECOND_BYTE`, and sets `ST_CONTROLLER_ACTUATOR_TIMER`
to the result divided by 16 plus 5. If `ST_CONTROLLER_ACTUATOR_MODE` is 1,
it also sets `ST_CONTROLLER_ACTUATOR_FIRST_BYTE` to 1. For nonpositive input,
it clears the second byte and, when the mode is 1, the first byte. A zero
setting leaves these values unchanged. For downward collision, the helper's
input is 0 through 128: zero clears the applicable bytes, while positive input
sets the second byte to 65 through 192.

[ST_UpdateControllerActuatorTimer](../scr/st/ST_UpdateControllerActuatorTimer.c),
called by [DrawScene](../scr/st/DrawScene.c), decrements a nonzero timer. If
the timer is already zero when called, it clears both actuator bytes. The
controller initialization passes the four-byte region containing those bytes
to `sub_8007CCC4`; that port function currently bypasses the `PadSetAct` call.

For `d > 0`:

- A negative vertical value is divided by two, rounding toward zero.
- A zero value becomes `d` if `d < 32`, otherwise 16; when `32 <= d <=
  1024`, the movement limit is also cleared.
- A positive value increases by 48 only if the collision query flags above
  the low four bits are zero. The movement limit is cleared if `d <= 1024`.

When `d == 0`, vertical movement and the byte at player offset `0x239` are
cleared. Other collision branches may change the applied displacement. These
are per-update stored integer values; the physical units have not been
established. With one spatial cell as one world unit, divide
positions and movement displacements by 4096 to obtain values with 12
fractional bits. See [STAGE_UNITS.md](STAGE_UNITS.md) for the extracted
geometry and the relationship between model, grid, and player coordinates.

Near the end of `ST_PlayerMoveWithCollision`,
[ST_UpdatePlayerCollisionYFlags](../scr/st/ST_UpdatePlayerCollisionYFlags.c)
compares query words `0x4C` and `0x58` with `ST_PLAYER_POSITION_Y`,
using signed 32-bit comparisons. For each query word other than the sentinel
`0x80000`, it writes 1 to `ST_PLAYER_FIRST_COLLISION_Y_FLAG` (`0x2EC`) or
`ST_PLAYER_SECOND_COLLISION_Y_FLAG` (`0x2F0`), respectively, if the query value is less than player Y, or 0 otherwise. A sentinel
leaves that flag unchanged. If byte `0x801B5924` is 10 and signed player
halfword `0x178` is `0x147`, the function clears `ST_PLAYER_SECOND_COLLISION_Y_FLAG` even if the second
query word is the sentinel. The game state machine reads this flag when
choosing subsequent behavior. **OPEN:** the physical meanings of these flags
and the special-case values have not been established.
