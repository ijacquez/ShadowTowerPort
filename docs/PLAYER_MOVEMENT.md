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

Within each main-loop iteration, `ST_UpdatePlayerState` runs before
[ST_UpdateLevelRecords](../scr/st/ST_UpdateLevelRecords.c) and
[ST_UpdateActiveCollisionRecords](../scr/st/ST_UpdateActiveCollisionRecords.c), in that order. The latter two
therefore update their contribution vectors after that iteration's player
movement update. Other code can also write those vectors.

## State dispatch and movement gating

On reaching its state dispatch, [ST_UpdatePlayerState](../scr/st/ST_UpdatePlayerState.c)
selects a path from `ST_PLAYER_STATE`. For states 0 and 1 it first calls
`ST_ProcessPlayerActionControls`. If the signed halfword at
`ST_LEVEL_STREAMING_STATUS_ADDRESS` is 1, or the byte at
`ST_PLAYER_UPDATE_GATE_ADDRESS` is nonzero, it skips the turning, movement,
and collision calls. Otherwise it calls those three functions in that order,
then clears `ST_PLAYER_DIRECTIONAL_MOVE_DISABLED`.

For state 16, `ST_ProcessPlayerActionControls` and the same three movement
calls occur without those two gates. Between
the movement and collision calls, if the word beginning at
`ST_PLAYER_VIEW_STATE_ROTATION_X` and the signed halfword
`ST_PLAYER_VIEW_STATE_ROTATION_Z` are both zero,
`ST_ResetPlayerStateAndViewRotationOffsets` clears the state and the three
state, transient, and additional view rotation components. The state-16 path clears
`ST_PLAYER_DIRECTIONAL_MOVE_DISABLED` after collision. States 5 and 6 clear
the movement limits, all three movement components, and the three contribution
sets before the shared post-dispatch work. **OPEN:** the in-game meanings of
the remaining state values are not established.

States 2, 3, and 7 through 15, and values above 16, dispatch directly to
shared post-dispatch work without calling the turning, movement, or collision
functions on that dispatch path. State 4 takes its own path; it does not call
those three functions there, though it can request streaming and clear movement
limits. Other functions in the shared work can still affect player state.

In the shared work, each of three bits in `ST_COLLISION_QUERY_RECORD_FLAGS`
is checked independently. If the bit is set and signed
`ST_COLLISION_QUERY_CURRENT_Y_RESULT` is at most `0x640` (0.390625 cells),
the state update calls `ST_ApplyPlayerDamageAndEffects` with multiplier
`0x1000` (1.0 in Q.12):

| Record flag | Third argument | First argument |
| --- | --- | --- |
| `ST_COLLISION_RECORD_EFFECT_7_FLAG` (`0x10`) | `0x7` | An 18-byte zeroed buffer. |
| `ST_COLLISION_RECORD_EFFECT_400_FLAG` (`0x20`) | `0x400` | An 18-byte zeroed buffer. |
| `ST_COLLISION_RECORD_EFFECT_INPUT_FLAG` (`0x8`) | `0` | An 18-byte zeroed buffer with `ST_COLLISION_RECORD_EFFECT_INPUT_HALFWORD_OFFSET` set to 150. |

Each call also receives a separate zeroed 16-byte output buffer. These flags
can cause more than one call in the same update; their in-game meanings are
**OPEN**.

The update gate is also used by the `ST_LEVEL_RECORD_TYPE` `0xE0` path in
[ST_UpdateLevelRecords](../scr/st/ST_UpdateLevelRecords.c). On a qualifying path with the gate
at zero, the routine sets it to 1 and clears the movement limits. With the
gate at 1, it synchronizes pending loads, sets
`ST_PLAYER_DIRECTIONAL_MOVE_DISABLED`, and examines the final eight active
collision records, starting at `ST_ACTIVE_COLLISION_RECORD_FINAL_EIGHT_ADDRESS`.
For each record whose first byte is not `0xFF`, it stores cosine and sine of
its signed `ST_ACTIVE_COLLISION_RECORD_PLAYER_TO_RECORD_ANGLE` halfword minus `0x400`,
divided by 256 toward zero, in
`ST_LEVEL_RECORD_DIRECTION_X_GP_OFFSET` and
`ST_LEVEL_RECORD_DIRECTION_Z_GP_OFFSET`. Each qualifying record replaces the
previous record's values. If at least one record qualifies, the gate becomes 2.
At gate 2 the routine clears the movement limits and copies the two stored
values to the X and Z quarter-contribution halfwords; its end-of-loop
damping, described below, then applies to the copied values. A subsequent condition
in the same slot loop can set the gate to 3. At gate 3 the routine clears the
stored direction values, sets the gate to 4, and requests level streaming.
[ST_CompleteLevelStreamingRequest](../scr/st/ST_CompleteLevelStreamingRequest.c) clears the gate,
`ST_LEVEL_STREAMING_COMPLETION_CLEARED_FLAG`, and the status halfword at
`ST_LEVEL_STREAMING_STATUS_ADDRESS`.
For each of five consecutive bytes starting at `ST_LEVEL_STREAMING_REQUEST_PRIMARY_ID`,
it copies the byte to the corresponding position starting at
`ST_LEVEL_STREAMING_APPLIED_ID_FIRST` unless the requested byte is `0xFF`.
`ST_UpdateLevelStreaming` sets the completion-cleared flag on one of its paths.
**OPEN:** the purpose of that flag and the
in-game meanings of these record types and gate states remain
unestablished.

For each nonempty active record, `ST_UpdateActiveCollisionRecords` sets
`ST_ACTIVE_COLLISION_RECORD_PLAYER_TO_RECORD_ANGLE` to
`(ratan2(recordZ - playerZ, recordX - playerX) - 0x400) & 0xFFF` using
32-bit wrapped differences. The main-loop order means the level-record
update runs before this calculation in the same iteration.

The `ST_LEVEL_RECORD_TYPE` `0xE0` path first calls
[ST_IsPlayerWithinGridBounds](../scr/st/ST_IsPlayerWithinGridBounds.c) with
`ST_LEVEL_RECORD_POSITION_X` and `ST_LEVEL_RECORD_POSITION_Z` shifted right
by 12, widths of one cell, and `ST_LEVEL_RECORD_POSITION_Y`. The helper
rounds each player X/Z position by adding `0x800`
(0.5 cells) and shifting right by 12, then checks that each result is at
least the supplied lower bound and strictly less than lower bound plus width.
It also checks that player Y is between the supplied Y minus `0x1000`
(1.0 cell) and the supplied Y, inclusive, unless the supplied Y equals the
sentinel `0x8000`. Additions and subtractions use 32-bit wrapping and
comparisons are signed. A nonzero result passes this first gate; a zero result
also proceeds when `ST_LEVEL_STREAMING_STATUS_ADDRESS` is nonzero.

[ST_IsPlayerWithinWorldBounds](../scr/st/ST_IsPlayerWithinWorldBounds.c)
uses the same vertical check and sentinel, but compares each player X/Z
position plus half its supplied width against world-coordinate bounds. The
half-width division rounds toward zero. `ST_LEVEL_RECORD_TYPE` values `0xED` and `0xEF` call
it with X and Z widths of `0xC00` (0.75 cells).

## View rotation

In the shared post-dispatch work, `ST_UpdatePlayerState` forms each view
rotation component by adding four unsigned halfwords. X uses `ST_PLAYER_PITCH`,
Y uses `ST_PLAYER_YAW`, and Z uses `ST_PLAYER_BASE_ROTATION_Z`; each also
uses the matching `ST_PLAYER_VIEW_STATE_ROTATION_*`,
`ST_PLAYER_VIEW_TRANSIENT_ROTATION_*`, and
`ST_PLAYER_VIEW_ADDITIONAL_ROTATION_*` components. It stores the low 16 bits
of each sum in `ST_PLAYER_VIEW_ROTATION_*` and clears the transient components.
It also stores the low 16 bits of `0x1000 - sum` in each corresponding
`ST_PLAYER_INVERSE_VIEW_ROTATION_*` component and passes those components
to [RotMatrixZYX](../scr/st/RotMatrixZYX.c), with the output at
`ST_PLAYER_VIEW_ROTATION_MATRIX`. **OPEN:** the full set of writers and in-game meanings of the state,
transient, and additional rotation components.

After updating vertical movement, `ST_PlayerMoveWithCollision` updates the
signed halfword `ST_PLAYER_VERTICAL_VIEW_PITCH_GP_OFFSET` relative to the
emulated global pointer. For vertical movement `v` greater than `0x80`
(0.03125 cells per update), it stores `(v - 0x80) / 2`; for `v` below
`-0x80` (-0.03125), it stores `(v + 0x80) / 2`. Otherwise it stores half
the previous halfword. Each division rounds toward zero. It copies the
result to `ST_PLAYER_VIEW_ADDITIONAL_ROTATION_X`, including when
`ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED` is nonzero.

Before forming the view rotation, `ST_UpdatePlayerState` calls
[ST_UpdateRandomPlayerViewRotation](../scr/st/ST_UpdateRandomPlayerViewRotation.c).
If signed `ST_PLAYER_RANDOM_VIEW_TIMER` is negative, that helper returns
without changing the transient rotations. Otherwise, when the timer is
divisible by three, it calls `rand_recomp` twice. For each result it subtracts
`0x4000` with 32-bit wrapping, then arithmetic-shifts right by
`(5 + (30 - timer) / 5) & 31`, where the division truncates toward zero.
The low halfwords become `ST_PLAYER_RANDOM_VIEW_X_GP_OFFSET` and
`ST_PLAYER_RANDOM_VIEW_Z_GP_OFFSET`. On other timer values, each existing
signed halfword is arithmetic-shifted right by one. Each resulting halfword
outside -2 through 2 is added to the matching X or Z transient rotation with
halfword wrapping. The helper then clamps raw transient values `0x801` through
`0xFBF` to `0xFC0`, and `0x41` through `0x7FF` to `0x40`, for both axes;
it decrements the timer with 32-bit wrapping. The reset routine sets the
timer to -1; [ST_ApplyPlayerDamageAndViewResponse](../scr/st/ST_ApplyPlayerDamageAndViewResponse.c)
can also set it, then clamps the result to -1 through 30. The complete set
of writers is **OPEN**.

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

With the supplied executable's lookup values, zero yaw, zero initial
movement contributions and vertical movement, and a zero result from each
directional collision query, those five updates also add the same successive
values to world Z. Starting at world Z zero, the positions immediately after
`ST_PlayerUpdateMovement` are `0x30`, `0x90`, `0x120`, `0x1E0`, and `0x2A0`
(0.01171875, 0.03515625, 0.0703125, 0.1171875, and 0.1640625 cells).
World X remains zero. This example describes that function's clear-query
path; later movement and collision processing can change the position.
Releasing the control under the same conditions gives Z displacements of
`0xA8`, `0x90`, `0x78`, `0x60`, `0x48`, `0x30`, `0x18`, and `0`
(0.041015625, 0.03515625, 0.029296875, 0.0234375, 0.017578125,
0.01171875, 0.005859375, and 0 cells). The final Z position is `0x540`
(0.328125 cells). On the first release update, the first square-root lookup
returns `0xA7` (0.040771484375 cells) for the accumulator magnitude, the
subsequent squared-component division gives `0xA9` (0.041259765625 cells),
and the second lookup returns `0xA8` (0.041015625 cells).

For the first update from idle under the same clear-query conditions, the
positive local Z control produces these world displacements at cardinal
`ST_PLAYER_YAW` values. The angle wraps at `0x1000` per full turn.

| Yaw | World X displacement | World Z displacement |
| --- | --- | --- |
| `0x000` | 0 | `0x30` (0.01171875 cells) |
| `0x400` | `-0x30` (-0.01171875 cells) | 0 |
| `0x800` | 0 | `-0x30` (-0.01171875 cells) |
| `0xC00` | `0x30` (0.01171875 cells) | 0 |

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
which clears vertical movement, the view adjustments, and the state,
transient, and additional view rotation components. It copies the two words
starting at `ST_PLAYER_PITCH` and `ST_PLAYER_BASE_ROTATION_Z` to the words
starting at `ST_PLAYER_VIEW_ROTATION_X` and `ST_PLAYER_VIEW_ROTATION_Z`,
respectively. It also clears both player collision Y flags and
`ST_COLLISION_QUERY_RECORD_FLAGS`.

When the helper's signed halfword result for the suppression pair is
not `-1`, `ST_UpdatePlayerCountersAndEffects` clears the movement and turn
limits via `ST_ClearPlayerMovementLimits`, and clears player movement
halfwords `ST_PLAYER_MOVE_X`, `ST_PLAYER_MOVE_Y`, and `ST_PLAYER_MOVE_Z`.
It also clears all three halfwords in
the vectors beginning at `ST_PLAYER_QUARTER_CONTRIBUTION_X_ADDRESS` and
`ST_PLAYER_HALF_CONTRIBUTION_X_ADDRESS`, as well as
`ST_COLLISION_QUERY_RECORD_VECTOR_SUM_X`, `ST_COLLISION_QUERY_RECORD_VECTOR_SUM_Y`,
and `ST_COLLISION_QUERY_RECORD_VECTOR_SUM_Z`. If the word containing the counter and
target is then zero, it clears bits `0x08` and `0x10` in player word `0x2C`.

[ST_RecalculatePlayerDerivedAttributes](../scr/st/ST_RecalculatePlayerDerivedAttributes.c) sets `ST_PLAYER_MOVE_PENALTY_TOTAL`
from a halfword calculated by
[ST_RecalculatePlayerAttributeScratch](../scr/st/ST_RecalculatePlayerAttributeScratch.c). The latter starts the halfword at
zero and iterates player halfword slots beginning at `ST_PLAYER_RECORD_SLOTS`. For slot indices
0 through 16, excluding 4, 5, 6, 11, 12, and 15, a slot other than `0xFFFF`
indexes a 44-byte record at `0x801C7810`; the unsigned record halfword at
`0x24` is added to the total with halfword wrapping. `ST_RecalculatePlayerDerivedAttributes` sets the
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

On paths in [ST_ApplyPlayerDamageAndEffects](../scr/st/ST_ApplyPlayerDamageAndEffects.c) that reach its
third-argument dispatch, values `0x08`, `0x10`, and `0x18` select the
suppression pair. The code sets `ST_PLAYER_MOVE_SUPPRESSION_TARGET` to 75. Depending
on bits `0x08` and `0x10` of player word `0x2C`, it either clears the
current counter and sets one of those bits, or sets the current counter to
20. Dispatch value `0x80` selects the reduction pair and sets its
target to 300. If bit `0x80` of player word `0x2C` is clear, the code clears
the current counter and sets the bit; otherwise, it sets the current counter
to 20. After this dispatch, the routine calculates limits using the unsigned
player fields `ST_PLAYER_EFFECT_CAP_ATTRIBUTE` and
`ST_PLAYER_EFFECT_CAP_PERCENT`; if a target interpreted as signed exceeds its calculated
limit, the routine replaces it with the low halfword of that limit. The
75 and 300 values above therefore are subject to this later cap. **OPEN:** the
in-game meanings of the two fields used to calculate the caps are not established.

`ST_RecalculatePlayerAttributeScratch` calculates 14 attribute values in
scratch memory. For each index, it starts a halfword sum at zero, then adds
the unsigned byte at that index in `ST_RECORD_ATTRIBUTE_VALUES` for each
selected record. Each addition wraps to a halfword. It adds the corresponding
halfword in `ST_PLAYER_BASE_ATTRIBUTES`, retains the low halfword, interprets
it as signed, and clamps it to 0 through 999. For this calculation the
selected slots are indices 0 through 3, 7 through 10, 13, 14, and 16;
`0xFFFF` indicates an empty slot.

`ST_RecalculatePlayerDerivedAttributes` derives
`ST_PLAYER_EFFECT_CAP_ATTRIBUTE` from element
`ST_EFFECT_CAP_ATTRIBUTE_INDEX` of that clamped array. It shifts the value
right by four bits and adds 50. This attribute is therefore between 50 and
112 for values produced by the described calculation. **OPEN:** the
in-game meaning of this attribute and its underlying base and record values.

For that attribute range, the limits calculated by
`ST_ApplyPlayerDamageAndEffects` can be expressed with integer division:

```text
base = ST_EFFECT_CAP_BASE_PERCENT
     + ST_EFFECT_CAP_BASE_PERCENT * (ST_EFFECT_CAP_REFERENCE_ATTRIBUTE
       - ST_PLAYER_EFFECT_CAP_ATTRIBUTE) / 59
suppression limit = (base * 75 / 100) * ST_PLAYER_EFFECT_CAP_PERCENT / 100
reduction limit   = (base * 300 / 100) * ST_PLAYER_EFFECT_CAP_PERCENT / 100
```

Each division discards its remainder before the next multiplication. For
example, attribute 80 and percent 100 give base 77, suppression limit 57,
and reduction limit 231. The suppression and reduction targets are capped
separately by these limits.

[ST_RecalculatePlayerRecordModifiers](../scr/st/ST_RecalculatePlayerRecordModifiers.c)
calculates `ST_PLAYER_EFFECT_CAP_PERCENT` from the same indexed 44-byte
records and player slots described above. For each selected record, it reads
three selectors from the low nibble of each of the first two bytes of
`ST_RECORD_PACKED_MODIFIERS` and from `ST_RECORD_THIRD_MODIFIER_CODE`.
Their corresponding values are the high nibble of each of those first two
bytes and `ST_RECORD_THIRD_MODIFIER_VALUE`. For each
selector equal to 5, it adds the corresponding value to a running sum and
caps that sum at 100. The stored byte is `100 - (sum * 80 / 100)`, with
integer division. With no matching selectors, it stores 100. **OPEN:** the
in-game meaning of selector 5 and the names of the indexed records.

[ST_SerializeGameState](../scr/st/ST_SerializeGameState.c) copies both
current/target pairs to buffer offsets `0x1880` through `0x188A`.
[ST_DeserializeGameState](../scr/st/ST_DeserializeGameState.c) restores the
same halfwords to the player. In [ST_ExecutePlayerActionCode](../scr/st/ST_ExecutePlayerActionCode.c),
`ST_PLAYER_ACTION_CLEAR_MOVE_SUPPRESSION` (`0x11F`) checks
`ST_PLAYER_EFFECT_MOVE_SUPPRESSION_MASK` (`0x18`) in `ST_PLAYER_EFFECT_FLAGS`; when
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

The port's [SquareRoot0](../scr/st/SquareRoot0.c) wrapper executes the
recompiled game routine, which updates the GTE leading-sign-bit registers
and reads a signed halfword relative to `ST_SQUARE_ROOT_TABLE_ADDRESS`;
for a negative signed input, a negative index
there addresses earlier emulated memory. The all-ones input takes the
routine's zero-result branch. Thus a wrapped negative squared sum does not
establish a nonnegative geometric distance.

For positive input `s`, the game routine clears the low bit of
the leading-sign-bit count to obtain `n`, then computes `index` by shifting
`s` right by `24 - n` if `n < 24`, or left by `n - 24` otherwise. It
returns `SQRT[index - 64] << ((31 - n) >> 1) >> 12`. Zero returns zero.
The supplied executable's 192 table halfwords at
`ST_SQUARE_ROOT_TABLE_ADDRESS` match the PsyCross `SQRT` table. The lookup
and integer shifts define the result; it is not specified as an exact
real-valued square root.

The rotation input's Y halfword is loaded from
`ST_PLAYER_ROTATION_INPUT_Y_STACK_OFFSET` in the movement function's stack
frame. The function allocates `0x70` bytes and does not write that halfword
before loading it; its preceding calls to `SquareRoot0` and `RotMatrix` do
not write it either. The address is `0x54` bytes below the stack pointer at
the call to `ST_PlayerUpdateMovement`. Each call in
[ST_UpdatePlayerState](../scr/st/ST_UpdatePlayerState.c) is immediately
preceded by [ST_PlayerUpdateTurning](../scr/st/ST_PlayerUpdateTurning.c),
whose stack writes and angle-comparison helper do not write that address.
One source is the table copy in
[ST_RecalculatePlayerLowRecordValueFlag](../scr/st/ST_RecalculatePlayerLowRecordValueFlag.c).
[ST_ProcessPlayerActionControls](../scr/st/ST_ProcessPlayerActionControls.c)
calls it when `ST_CONTROL_ACTION_SELECTION` is 2 or 3,
`ST_PLAYER_ACTION_STATE_PAIR` is `0xFFFFFFFF`, and the corresponding
`ST_PLAYER_ACTION_FIRST_RECORD_SLOT` or `ST_PLAYER_ACTION_SECOND_RECORD_SLOT`
is not `0xFFFF`. The caller allocates `0x20` bytes and the recalculation
function allocates `0x48`. The latter copies the second word of
`ST_LOW_RECORD_VALUE_SLOT_POINTERS` to its frame offset `0x14`; this word
overlaps `ST_PLAYER_ROTATION_INPUT_Y_STACK_OFFSET` in the later movement
frame. In the supplied `ST.EXE`, that table word is `0x80198F64`, so its low
halfword is `0x8F64` (-28828). If the table retains that value and both
`ST_PLAYER_FIRST_ACTION_REPEAT` and `ST_PLAYER_SECOND_ACTION_REPEAT` are
`0xFF` after the recalculation, the remaining control and turning code does
not overwrite that halfword before the movement call.

**OPEN:** the final prior writer on other paths and the halfword's intended
role are not established. In the supplied executable, the zero-angle
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
though the horizontal results do not. The directional collision query calls
`ST_QueryLevelGeometry`. When that query reaches a vertex transformation,
[ST_TransformCandidateVertices](../scr/st/ST_TransformCandidateVertices.c)
loads its own rotation matrix, translation and vertex before issuing MVMVA.
For that command, the GTE implementation clears the previous flags and
computes new MAC words and IR low halfwords from those loaded values. The
function stores the MAC words as the transformed vertex, so the prior Y
result and flags do not contribute to it. In the port's GTE implementation,
the command leaves the upper half of each IR register unchanged until a
`gte_mfc2` read sign-extends its low halfword. **OPEN:**
the final prior writer on other paths and later uses of this GTE state on
paths without a vertex transformation have not been established.

The rotation uses `-yaw` and writes GTE results back to the stack before
computing another angle and magnitude. The later code updates each signed
movement halfword `ST_PLAYER_MOVE_X`, `ST_PLAYER_MOVE_Y`, and
`ST_PLAYER_MOVE_Z` by subtracting one quarter of its value, rounded toward zero. It
then clears a resulting halfword in the range -3 through 3. It adds one
quarter of `ST_PLAYER_QUARTER_CONTRIBUTION_X_ADDRESS` to X and one quarter of
`ST_PLAYER_QUARTER_CONTRIBUTION_Z_ADDRESS` to Z, rounding toward zero. It also adds half
`ST_PLAYER_HALF_CONTRIBUTION_X_ADDRESS` and `ST_PLAYER_HALF_CONTRIBUTION_Z_ADDRESS`
to X and Z, respectively, again rounding toward zero. These names identify
the addresses of signed halfwords; each result is stored as a
halfword. [ST_UpdateLevelRecords](../scr/st/ST_UpdateLevelRecords.c) adds cosine and sine
results divided by 32 to the first pair on one path. At the end of its
240-record loop, it halves the signed halfwords at
`ST_PLAYER_QUARTER_CONTRIBUTION_X_ADDRESS`,
`ST_PLAYER_QUARTER_CONTRIBUTION_Y_ADDRESS`, and
`ST_PLAYER_QUARTER_CONTRIBUTION_Z_ADDRESS`, rounding toward zero, and clears each
result in the range -3 through 3. On a path following its 64-record loop,
[ST_UpdateActiveCollisionRecords](../scr/st/ST_UpdateActiveCollisionRecords.c) halves the signed halfwords at
`ST_PLAYER_HALF_CONTRIBUTION_X_ADDRESS`, `ST_PLAYER_HALF_CONTRIBUTION_Y_ADDRESS`,
and `ST_PLAYER_HALF_CONTRIBUTION_Z_ADDRESS` with an arithmetic right
shift, then clears each result in the range -3 through 3. Thus negative odd
values round down in the second set, unlike the first set. **OPEN:** the
complete set of writers and the meanings of these values have not been
established.

During that 64-record loop, `ST_UpdateActiveCollisionRecords` skips callback
dispatch for records whose `ST_ACTIVE_COLLISION_RECORD_ID_OR_EMPTY` is
`0xFF`. For a record in state 1, it calls the function at
`ST_LEVEL_CALLBACK_ACTIVE_RECORD_PERIODIC` when the low two bits of
`ST_ACTIVE_RECORD_UPDATE_COUNTER_ADDRESS` and
`ST_ACTIVE_RECORD_PROCESSED_COUNTER_ADDRESS` match, then calls the function
at `ST_LEVEL_CALLBACK_ACTIVE_RECORD_UPDATE`. For state 5 it calls only the
latter. The function pointers come from the table addressed by
`ST_LEVEL_STREAMING_CALLBACK_TABLE_OFFSET`. **OPEN:** the complete set of
callback targets and their writes to the contribution vectors.

Before dispatch, [ST_SetCurrentActiveCollisionRecord](../scr/st/ST_SetCurrentActiveCollisionRecord.c)
sets `ST_CURRENT_ACTIVE_COLLISION_RECORD_ADDRESS` to the record and
`ST_CURRENT_ACTIVE_COLLISION_CLASS_ADDRESS` to its selected class. In stage
0's slot-2 resource, the periodic and update entries contain
`ST_STAGE_ZERO_ACTIVE_RECORD_PERIODIC_CALLBACK` and
`ST_STAGE_ZERO_ACTIVE_RECORD_UPDATE_CALLBACK`, respectively. The port executes
these addresses through its MIPS interpreter. In the supplied `FDAT.T`, the
slot-2 resources for level indices 32 and 40 are byte-for-byte identical to
the stage-zero resource, though their other level data can differ. The update callback begins by
adding `ST_ACTIVE_COLLISION_RECORD_UPDATE_STEP` to
`ST_ACTIVE_COLLISION_RECORD_UPDATE_ACCUMULATOR` and storing the low halfword,
then calls `ST_STAGE_ZERO_ACTIVE_RECORD_UPDATE_COUNTDOWN`. This routine reads
`ST_ACTIVE_COLLISION_RECORD_UPDATE_COUNTDOWN` as signed. If it is negative,
the routine returns zero. Otherwise it subtracts the absolute signed
`ST_ACTIVE_COLLISION_RECORD_UPDATE_STEP`, stores the low halfword as the
countdown, and tests that stored halfword as signed.

If the resulting countdown is negative and
`ST_ACTIVE_COLLISION_RECORD_TIMER_BEHAVIOR` is zero, the routine returns 1;
when the signed update accumulator is at least 4095, it first calls
[ST_EnterActiveCollisionRecordStateThree](../scr/st/ST_EnterActiveCollisionRecordStateThree.c).
If the resulting countdown is negative and the timer behavior is nonzero,
it sets the countdown to -1, clears bit `0x20` in
`ST_ACTIVE_COLLISION_RECORD_TIMER_FLAGS`, and returns zero. If the countdown
is nonnegative, the signed accumulator is at least 4096, and the record is in
state 5, it sets the accumulator to 4095 and returns zero; otherwise it
returns zero without that adjustment. The state-three routine sets
`ST_ACTIVE_COLLISION_RECORD_STATE` to 3 and recalculates the record position
from its placement. **OPEN:** the callbacks' complete effects and the in-game
meanings of these record fields.

If the countdown routine returns 1, the update callback returns immediately.
Otherwise it reads the signed step and accumulator. It sets
`ST_ACTIVE_COLLISION_RECORD_CALLBACK_CONTROL` to `0xFF` and calls the
periodic callback when the step is nonnegative and the accumulator is at
least 4096, or the step is negative and the accumulator is at most zero.
Afterward, if `ST_ACTIVE_COLLISION_RECORD_CALLBACK_DATA` is nonzero, it calls
another stage-zero routine and sets the control byte to `0xF0`.

That other routine is `ST_STAGE_ZERO_ACTIVE_RECORD_BEHAVIOR_CALLBACK`. For
`ST_ACTIVE_COLLISION_RECORD_CALLBACK_BEHAVIOR` equal to
`ST_STAGE_ZERO_ACTIVE_RECORD_IMPULSE_BEHAVIOR`, it can write the player's
half-contribution vector. It begins with magnitude zero and checks two
intervals from the callback data. An interval qualifies when its unsigned
start is not `0xFFFF` and the signed update accumulator lies inclusively
between its unsigned start and end. For a qualifying interval, magnitude is
the low 32 bits of its signed scale multiplied by the absolute signed update
step, divided by `end - start` with signed division toward zero. The second
interval replaces the first magnitude if both qualify. A zero divisor reaches
a division break. The corresponding fields are named
`ST_STAGE_ZERO_BEHAVIOR_FIRST_*` and `ST_STAGE_ZERO_BEHAVIOR_SECOND_*` in
[ST_StageZeroActiveRecordBehavior.h](../scr/st/ST_StageZeroActiveRecordBehavior.h).

The vector write requires the callback-data word and halfword gates to equal
`-1` and `0xFFFF`, respectively; the record's
`ST_ACTIVE_COLLISION_RECORD_IMPULSE_APPLIED` to be zero;
`ST_ACTIVE_COLLISION_RECORD_HIT_PLAYER_FLAG` to equal 1; magnitude
to be nonzero; and the signed `ST_ACTIVE_COLLISION_RECORD_PLAYER_ANGLE_DISTANCE`
to lie between the negative and positive unsigned
`ST_STAGE_ZERO_BEHAVIOR_ANGLE_LIMIT`, inclusive. It calls
`ST_STAGE_ZERO_ACTIVE_RECORD_APPLY_PLAYER_EFFECTS`, sets the impulse-applied
halfword to 1, and adds the low 32-bit products of magnitude with
`rcos(angle - 0x400)` and `rsin(angle - 0x400)` to the X and Z
half-contribution values after an arithmetic right shift by 11. Here `angle`
is `ST_ACTIVE_COLLISION_RECORD_PLAYER_TO_RECORD_ANGLE`, read after the effects
call. The effects helper calls
[ST_ApplyPlayerDamageAndEffects](../scr/st/ST_ApplyPlayerDamageAndEffects.c);
it passes `0x1000` (1.0 in Q.12) and dispatch value
`(ST_STAGE_ZERO_BEHAVIOR_EFFECT_FLAGS >> 8) & 0xFF`. The latter can select
the movement-related counter branches described above.

On the state-1 update path, the callback reads the current
`ST_ACTIVE_COLLISION_RECORD_HIT_PLAYER_FLAG` before the updater clears it.
The displacement resolver sets it to 1 when its collision result contains
`ST_COLLISION_RESULT_PLAYER_HIT`. That result bit is produced by
[ST_QueryPlayerCollision](../scr/st/ST_QueryPlayerCollision.c). A player
collision in the resolver can therefore supply the flag for a later callback;
other writes before that callback can also affect its value.

One routine called by that stage-zero code,
`ST_STAGE_ZERO_SET_ACTIVE_RECORD_HORIZONTAL_DISPLACEMENT`, writes the current
record's `ST_ACTIVE_COLLISION_RECORD_DISPLACEMENT_X` and `_Z`. For its signed
angle and magnitude arguments, it multiplies the magnitude by `rcos(angle)`
and `rsin(angle)`, respectively; each low 32-bit product is arithmetically
shifted right by 12 and stored as a halfword.

Before active-record callbacks, `ST_UpdateActiveCollisionRecords` clears the
record's three displacement halfwords. On the state-1 path after the update
callback, if any displacement is nonzero or the signed
`ST_ACTIVE_COLLISION_RECORD_COLLISION_Y_INPUT` is nonzero, it calls
[ST_ResolveActiveCollisionRecordDisplacement](../scr/st/ST_ResolveActiveCollisionRecordDisplacement.c).
That routine uses `ST_RunCollisionQuery` and can change the displacement.
Unless `ST_PLAYER_EFFECT_BLOCK_ACTIVE_RECORD_POSITION_UPDATE` is set, the
update then adds each signed displacement halfword to its corresponding
record position with 32-bit wrapping. These record positions are subsequently
used by `ST_QueryActiveCollisionRecords` when checking player movement.
**OPEN:** the complete set of displacement writers.

On the first query in the resolver, a nonzero
`ST_COLLISION_QUERY_CANDIDATE_COUNT` clears
`ST_ACTIVE_COLLISION_RECORD_NO_CANDIDATE_FLAG` if it is exactly 1, then
copies `ST_COLLISION_QUERY_SELECTED_PLACEMENT_INDEX` into
`ST_ACTIVE_COLLISION_RECORD_SELECTED_PLACEMENT_INDEX`. With no candidates,
it sets that flag to 1 only if it was zero and retains the previous selected
index. It indexes `ST_LEVEL_PLACEMENT_TABLE_ADDRESS` with that index and
compares `ST_LEVEL_PLACEMENT_FILTER_VALUE` with
`ST_ACTIVE_COLLISION_RECORD_PLACEMENT_FILTER`. If the bytes differ, it clears
the working X and Z displacement and continues to the vertical response
without processing the query's horizontal result bits. The candidate index
is initialized from the first candidate by `ST_QueryLevelGeometry` and can
be updated by its three- and four-vertex collision tests. **OPEN:** the
in-game meanings of the two compared bytes and the no-candidate flag.

In the resolver, result bits `ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD` and
`ST_COLLISION_RESULT_SECOND_NORMAL_THRESHOLD` take precedence over the
player-hit response, in that order. For either normal, let `R` be its query
result, `N` its signed X/Z normal, and `E` the record's unsigned horizontal
extent. The resolver replaces working X/Z with the low halfwords of
`(1 - (R - E)) * N / 4096`, using wrapped 32-bit arithmetic and division
toward zero. On the first occurrence of that particular normal bit, it also
sets `t = (ratan2(Nz, Nx) + 0x400) & 0xFFF` and stores `t` in
`ST_ACTIVE_COLLISION_RECORD_COLLISION_CORRECTION_ANGLE`. It projects the
original horizontal displacement magnitude onto that direction with
`p = originalMagnitude * rcos(originalDirection - t) / 4096`, then adds
`rcos(t) * p / 4096` and `rsin(t) * p / 4096` to working X and Z and
repeats the query. Each product uses its low 32 bits, each division rounds
toward zero, and each stored working component is a halfword. If the same
normal bit occurs again, it keeps the newly calculated working X/Z and
continues to the vertical response without another query.

When the result contains `ST_COLLISION_RESULT_ACTIVE_RECORD_HIT`,
`ST_COLLISION_RESULT_INDEXED_RECORD_HIT`, or
`ST_COLLISION_RESULT_PLAYER_HIT`, the resolver calls
`ST_SetCollisionReference`. For a player hit it also sets
`ST_ACTIVE_COLLISION_RECORD_HIT_PLAYER_FLAG`, and the reference selector
chooses the player position. Otherwise the reference selector uses the
available active or indexed record as described below; it can retain an old
reference if neither index is available. The resolver clears the
working X/Z displacement and, on the first such response, computes
`t = (ratan2(recordZ - referenceZ, recordX - referenceX) + 0x400) & 0xFFF`.
It computes `p` from the original horizontal displacement magnitude times
`rcos(originalDirection - t)`, using the low 32-bit product divided by 4096
toward zero. If `p` is nonzero, it adds `rcos(t) * p / 4096` to working X
and `rsin(t) * p / 4096` to working Z, again using low 32-bit products and
division toward zero; each component is stored as a halfword. It then repeats
the collision query. If the reference-response path is reached a second time,
it leaves working X/Z at zero and continues to the vertical response. The
resolver allows at most three nonzero-result correction passes before that
vertical response. A nonzero result containing none of the horizontal bits
above proceeds directly to the vertical response.

For the vertical response, the resolver sets `D` to
`ST_COLLISION_QUERY_PROPOSED_Y_RESULT` minus the unsigned
`ST_ACTIVE_COLLISION_RECORD_VERTICAL_EXTENT` and stores the wrapped 32-bit
result in `ST_ACTIVE_COLLISION_RECORD_VERTICAL_DIFFERENCE`. Its initial
working Y is the original Y displacement when
`ST_ACTIVE_COLLISION_RECORD_VERTICAL_MODE` is 2; otherwise it is
`ST_ACTIVE_COLLISION_RECORD_COLLISION_Y_INPUT`. If the query result contains
`ST_COLLISION_RESULT_VERTICAL_THRESHOLD`, it first adds `D` to working Y and
stores the low halfword. It then applies these rules:

- Modes 1 and 2 clear working Y and leave the Y input unchanged.
- For positive `D` in modes other than 4, if `D` exceeds the unsigned
  vertical extent and the mode is not 8, clear working X/Z. Otherwise, if
  the signed Y input is positive and working Y exceeds `D`, set working Y
  to the low halfword of `D` and clear the Y input. If the Y input is zero
  and `D` is less than the signed `ST_ACTIVE_COLLISION_CLASS_VERTICAL_STEP`,
  add `D` to working Y and clear the Y input. In the remaining cases add the
  class vertical step to the Y input, storing the low halfword.
- In the remaining modes and values of `D`, a nonnegative `D` clears working
  Y and the Y input. For negative `D`, if wrapped `-D` is less than half the
  unsigned vertical extent, add `D` to working Y and clear the Y input;
  otherwise clear working X/Z.

Working components are halfwords, comparisons are signed unless specified,
and 32-bit additions and subtractions wrap. The final working components are
written back to the active-record displacement vector. The in-game meaning of
the vertical modes and class step remains **OPEN**.

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
also clears vertical movement, `ST_PLAYER_STATE`, and
`ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED`, clears
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
enables checks with `ST_COLLISION_QUERY_INPUT_FLAGS` (`0x00`) set to
`ST_COLLISION_QUERY_PLAYER_MOVEMENT_CHECKS` (`0x432`).
If `ST_PLAYER_MOVE_MAGNITUDE` is nonzero and `ST_PLAYER_DIRECTIONAL_MOVE_DISABLED` is zero,
the routine skips this query path and continues at the later vertical update.
**OPEN:** the exact collision shape and the meaning of both dimensions have
not been established. [ST_RunCollisionQuery](../scr/st/ST_RunCollisionQuery.c) dispatches the enabled checks; among
them, [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c) searches level geometry. The query returns a bitmask and
stores additional collision data in the shared query structure.

For queries made by `ST_PlayerMoveWithCollision`, `ST_COLLISION_QUERY_DIRECTION` (`0x20`) is
set to `ST_COLLISION_QUERY_DIRECTION_FROM_DISPLACEMENT` (`9999`).
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
When that halfword is not `ST_COLLISION_QUERY_DIRECTION_FROM_DISPLACEMENT`, the geometry routine uses it for the
cosine and sine terms and uses the query's Y displacement without adding
`0x640` to that vector component.

### Query dispatcher input and output flags

[ST_RunCollisionQuery](../scr/st/ST_RunCollisionQuery.c) reads a 32-bit
input mask `ST_COLLISION_QUERY_INPUT_FLAGS`. It initializes the result to zero and handles
the following input bits in order:

| Input bit | Operation | Result contribution |
| --- | --- | --- |
| `ST_COLLISION_QUERY_CHECK_LEVEL_GEOMETRY` (`0x2`) | Calls [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c). | Uses the helper's return value as the initial result. |
| `ST_COLLISION_QUERY_CHECK_ACTIVE_RECORDS` (`0x10`) | Calls [ST_QueryActiveCollisionRecords](../scr/st/ST_QueryActiveCollisionRecords.c), passing `2` as its second argument if `ST_COLLISION_QUERY_EXPAND_ACTIVE_RECORD_EXTENT` (`0x400`) is set, otherwise `0`. | ORs `ST_COLLISION_RESULT_ACTIVE_RECORD_HIT` (`0x10`) if the helper returns anything other than `-1`. |
| `ST_COLLISION_QUERY_CHECK_ACTIVE_RECORDS_WITH_OFFSETS` (`0x40`) | Calls [ST_QueryActiveCollisionRecordsWithOffsets](../scr/st/ST_QueryActiveCollisionRecordsWithOffsets.c). | ORs `ST_COLLISION_RESULT_ACTIVE_RECORD_HIT` if the helper returns anything other than `-1`. |
| `ST_COLLISION_QUERY_CHECK_INDEXED_RECORDS` (`0x20`) | Calls [ST_QueryIndexedCollisionRecords](../scr/st/ST_QueryIndexedCollisionRecords.c). | ORs `ST_COLLISION_RESULT_INDEXED_RECORD_HIT` (`0x40`) if the helper returns anything other than `-1`. |
| `ST_COLLISION_QUERY_CHECK_PLAYER` (`0x80`) | Calls [ST_QueryPlayerCollision](../scr/st/ST_QueryPlayerCollision.c). If `ST_COLLISION_QUERY_EXPAND_PLAYER_EXTENT` (`0x800`) is also set, first adds `0x200` (0.125 cells) to `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER`. | ORs `ST_COLLISION_RESULT_PLAYER_HIT` (`0x80`) if the helper returns anything other than `-1`. |

The three helpers for input bits `0x10`, `0x40`, and `0x20` have their
return values stored in `ST_COLLISION_QUERY_ACTIVE_RECORD_INDEX`,
`ST_COLLISION_QUERY_SECOND_ACTIVE_RECORD_INDEX`, and
`ST_COLLISION_QUERY_INDEXED_RECORD_INDEX`, respectively; if the corresponding input bit is clear, the dispatcher stores
`-1` at that address. After the geometry helper, the dispatcher adds the
unsigned `ST_COLLISION_QUERY_VERTICAL_PARAMETER` to `ST_COLLISION_QUERY_POSITION_Y` for the
remaining checks. Before returning it subtracts the then-current unsigned
`ST_COLLISION_QUERY_VERTICAL_PARAMETER`, stores the final mask in
`ST_COLLISION_QUERY_RESULT_FLAGS` (`0x801B59BC`), and returns it.
For the movement routines' `ST_COLLISION_QUERY_PLAYER_MOVEMENT_CHECKS` mask, the enabled checks are
`ST_COLLISION_QUERY_CHECK_LEVEL_GEOMETRY`, `ST_COLLISION_QUERY_CHECK_ACTIVE_RECORDS`,
and `ST_COLLISION_QUERY_CHECK_INDEXED_RECORDS`; `ST_COLLISION_QUERY_EXPAND_ACTIVE_RECORD_EXTENT` selects second argument `2` for
`ST_QueryActiveCollisionRecords`. With that second argument, the latter adds `0x200` (0.125 cells) to the
sum of `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER` and a candidate's halfword `0x2E` before its
horizontal comparison. The candidate records begin at `0x801BA138`, have a
stride of 212 bytes, and the scan examines at most 64 records. These
operations identify the result bits' producers; the
physical meanings of the helpers and their results are **OPEN**.

When `ST_SetCollisionReference` is called after the query, result bit
`ST_COLLISION_RESULT_PLAYER_HIT` selects the player position. Otherwise, a
a `ST_COLLISION_QUERY_ACTIVE_RECORD_INDEX` value other than `-1` selects
an active-record position; if that index is `-1`, an
`ST_COLLISION_QUERY_INDEXED_RECORD_INDEX` other than `-1` selects a level-record
position. If neither index is available, the routine leaves the previous
reference position in place. It does not use
`ST_COLLISION_QUERY_SECOND_ACTIVE_RECORD_INDEX` to select the reference.

`ST_QueryPlayerCollision` subtracts the query X/Z position from the player
X/Z position, rejecting a difference outside `[-0x2800, 0x2800]` (-2.5 to
2.5 cells) on either axis. It calculates the horizontal distance and the
angle of that difference. It subtracts from the distance the low 32-bit
product of the signed query magnitude and the cosine of query direction minus
that angle, divided by 4096 toward zero. The result must be strictly less than
the unsigned query horizontal parameter plus `0x100` (0.0625 cells).
For the vertical check, absent 32-bit wrap, the query Y must be in the
inclusive interval from player Y to player Y plus `0x640` (0.390625 cells)
plus the unsigned query vertical parameter. A match returns the horizontal
distance; a rejection returns -1.

`ST_QueryActiveCollisionRecordsWithOffsets` checks up to 64 active records,
skipping records outside state 1, records whose filter flags intersect
`ST_ACTIVE_COLLISION_RECORD_FILTER_MASK_ADDRESS`, and the current active
record. A record with `ST_ACTIVE_COLLISION_RECORD_FLAG_ALTERNATE_Y` is also
skipped when its signed `ST_ACTIVE_COLLISION_RECORD_LOCAL_X` equals
`ST_ACTIVE_RECORD_CURRENT_INDEX_ADDRESS`. Otherwise its test position uses
record X/Z and record Y minus the unsigned vertical extent plus signed
`ST_ACTIVE_COLLISION_RECORD_LOCAL_Z`. If that flag is clear and
`ST_ACTIVE_COLLISION_RECORD_FLAG_POSITION_OFFSET` is set, it calls
`ST_ComputeActiveCollisionRecordOffset` and adds the returned vector to the record position. With
neither flag it uses the record position directly. All position arithmetic
wraps to 32 bits. It passes this position, the query position, the sum of
the two unsigned horizontal parameters, and both unsigned vertical
parameters to
[ST_QueryHorizontalDistanceWithVerticalBounds](../scr/st/ST_QueryHorizontalDistanceWithVerticalBounds.c).
It returns the first record index for which that helper does not return -1,
or -1 if none matches. **OPEN:** the in-game meaning of these record flags.

[ST_ComputeActiveCollisionRecordOffset](../scr/st/ST_ComputeActiveCollisionRecordOffset.c)
asks [ST_SampleResourceVector](../scr/st/ST_SampleResourceVector.c) for vector
index zero, using `ST_ACTIVE_COLLISION_RECORD_RESOURCE_SLOT + 0x80`,
`ST_ACTIVE_COLLISION_RECORD_RESOURCE_VECTOR_SELECTOR`, and the signed
`ST_ACTIVE_COLLISION_RECORD_UPDATE_ACCUMULATOR`. If sampling succeeds, it
multiplies each signed sampled halfword by its corresponding signed
`ST_ACTIVE_COLLISION_RECORD_SCALE_*` halfword, takes the low 32-bit product,
arithmetic-shifts it right by 12, and stores the low halfword. If sampling
fails, it instead uses `(0, -verticalExtent >> 1, -horizontalExtent)`,
with the extents read as unsigned halfwords and results stored as halfwords.
The Y shift rounds a negative odd value downward.

The offset helper passes that vector and the record's X, Y, and Z rotation
halfwords to [ST_ApplyCompositeRotationToVector](../scr/st/ST_ApplyCompositeRotationToVector.c),
after adding `0x800` to Y and storing its low halfword. That function builds
the matrix with [ST_BuildCompositeRotationMatrix](../scr/st/ST_BuildCompositeRotationMatrix.c)
and passes it and the vector to `ApplyMatrix`, which writes three words. The
matrix builder constructs X, Y, and Z matrices with the named axis helpers,
uses `MulMatrix` to combine X and Z, then `MulMatrix2` to combine Y with that
result. These matrix operations use the GTE. The sampler's resource fields
and interpolation are described in [FILE_FORMATS.md](FILE_FORMATS.md).
**OPEN:** the full resource vector format and in-game purpose of the sampled vector.

The distance helper first rejects either wrapped signed X/Z difference if it
is outside `[-radius, radius]`. Unless query Y equals the sentinel `0xFFFF`,
it checks vertical overlap using the supplied candidate and query extents:
when candidate Y is below query Y numerically, candidate Y must be at least
query Y minus the query extent; otherwise query Y must be at least candidate Y
minus the candidate extent. It arithmetic-shifts both horizontal differences
right by 3, takes `SquareRoot0` of their wrapped squared sum, and shifts the
result left by 3. It returns this distance if it is at most the signed radius,
or -1 otherwise. This check quantizes negative and positive differences by
arithmetic shift before calculating distance.

### Active-record collision check for movement

[ST_UpdateLevelStreaming](../scr/st/ST_UpdateLevelStreaming.c) copies the
class table and passes the following length-prefixed section to
[ST_LoadActiveCollisionRecords](../scr/st/ST_LoadActiveCollisionRecords.c).
The loader examines 64 successive 24-byte entries. For an entry whose first
byte is not `0xFF`, it copies `ST_ACTIVE_COLLISION_INPUT_CLASS_INDEX`,
`ST_ACTIVE_COLLISION_INPUT_PLACEMENT_INDEX`, and the three
`ST_ACTIVE_COLLISION_INPUT_LOCAL_*` halfwords into the active record. It calls
[ST_InitializeActiveCollisionRecordClass](../scr/st/ST_InitializeActiveCollisionRecordClass.c),
which indexes 192-byte class entries by the class index and copies the
horizontal extent, vertical extent, and filter-flag word into the record.

[ST_SetActiveCollisionRecordPositionFromPlacement](../scr/st/ST_SetActiveCollisionRecordPositionFromPlacement.c)
indexes the 12-byte placement table using the record's placement index. For
each axis, it shifts the unsigned placement cell byte left by 12 and adds the
record's signed local halfword. It stores the results in
`ST_ACTIVE_COLLISION_RECORD_POSITION_X`, `_Y`, and `_Z`. This position
calculation does not use `ST_LEVEL_PLACEMENT_Y_SUBCELL`; the geometry origin
calculation in `ST_DecodeCandidateOrigins` does use it.

[ST_QueryActiveCollisionRecords](../scr/st/ST_QueryActiveCollisionRecords.c)
examines up to 64 records at `ST_ACTIVE_COLLISION_RECORD_TABLE_ADDRESS`, each 212 bytes long. With the
movement query's second argument of 2, a record is considered only if
`ST_ACTIVE_COLLISION_RECORD_STATE` equals 1, its
`ST_ACTIVE_COLLISION_RECORD_FILTER_FLAGS` has no bit in common with the byte
at `ST_ACTIVE_COLLISION_RECORD_FILTER_MASK_ADDRESS`, and its address differs
from the pointer stored at `ST_CURRENT_ACTIVE_COLLISION_RECORD_ADDRESS`.
Its `ST_ACTIVE_COLLISION_RECORD_CLASS_INDEX` selects a 192-byte entry in
`ST_ACTIVE_COLLISION_CLASS_TABLE_ADDRESS`; the entry's unsigned
`ST_ACTIVE_COLLISION_CLASS_HORIZONTAL_EXTENT` must be nonzero.
[ST_SetCurrentActiveCollisionRecord](../scr/st/ST_SetCurrentActiveCollisionRecord.c)
sets the current record pointer to its record argument, or clears it for a
null argument. [ST_ResetActiveCollisionRecords](../scr/st/ST_ResetActiveCollisionRecords.c)
clears the filter-mask byte while resetting the records.

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

For example, with zero query magnitude, horizontal parameter `0x100`
(0.0625 cells), and record extent `0x20` (0.0078125 cells), the strict
threshold is `0x320` (0.1953125 cells). For a record differing only in X,
the supplied square-root table returns `0x31F` (0.195068359375 cells) for
both `0x320` and `0x321` (0.1953125 and 0.195556640625 cells) of separation,
so both pass the horizontal comparison. At separation `0x322`
(0.19580078125 cells), it returns `0x321` (0.195556640625 cells), which
fails the comparison. Eligibility and vertical checks still apply.

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
position and the query loop ends. For a nonzero result,
`ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD` applies the current candidate even
when other result bits are set. The routine then checks those other bits;
normal correction and another query can result in additional position changes.
The two normal-threshold bits select separate horizontal correction paths. These use
query data and the original movement direction to form another X/Z candidate,
which is queried again. The routine calculates angles and perpendicular
directions; the precise meaning of the result bits and query fields is not
fully established. The routine tracks each of
these two correction cases and applies the current candidate if that same
case is encountered again.

For example, suppose the current X candidate is `0x64` (0.0244140625
cells), the first result contains both `ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD`
and `ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD`, the first normal result is
`0x100` (0.0625 cells), and its X component is `0x1000` (1.0). The first
result adds `0x64` to player X and produces a replacement X of `0x1`
(0.000244140625 cells). If the tangent contribution is zero and the next
query returns zero, the replacement is also applied, for a total X change
of `0x65` (0.024658203125 cells).

## Directional collision pass

[ST_ApplyDirectionalMovementWithCollision](../scr/st/ST_ApplyDirectionalMovementWithCollision.c)
takes the angle and magnitude computed by
`ST_PlayerUpdateMovement`. It starts with `x = rcos(angle) * magnitude / 4096`
and `z = rsin(angle) * magnitude / 4096`, using the low 32 bits of each
product and rounding the signed division toward zero. These working values
are `ST_DIRECTIONAL_WORK_X` and `ST_DIRECTIONAL_WORK_Z` in the function's
stack frame. It copies the signed vertical-movement halfword into
`ST_DIRECTIONAL_WORK_Y`. It initializes the query at
`ST_COLLISION_QUERY_BASE_ADDRESS` from the player position, with
`ST_COLLISION_QUERY_PLAYER_MOVEMENT_CHECKS`,
`ST_COLLISION_QUERY_HORIZONTAL_PARAMETER` set to `0x100` (0.0625 cells), and
`ST_COLLISION_QUERY_VERTICAL_PARAMETER` set to `0x640` (0.390625 cells).
Before each query, it writes the low halfwords of `x + ST_PLAYER_MOVE_X`, `y`,
and `z + ST_PLAYER_MOVE_Z` into `ST_COLLISION_QUERY_DISPLACEMENT_X`,
`ST_COLLISION_QUERY_DISPLACEMENT_Y`, and `ST_COLLISION_QUERY_DISPLACEMENT_Z`.
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
- Otherwise, if `ST_COLLISION_RESULT_OBJECT_HITS` is nonzero, the function
  calls `ST_SetCollisionReference` and passes the player-to-reference X/Z
  difference to `VectorNormalS`. Let `N` be the resulting signed halfwords,
  `D` be `ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_DISTANCE`, and `E` be
  `ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_EXTENT`. It computes
  `C = 1 - (D - E - margin)` with wrapped 32-bit arithmetic, where the margin
  is `ST_DIRECTIONAL_ACTIVE_RECORD_MARGIN` (`0x300`, 0.1875 cells) when the
  active-record hit bit is set, or `ST_DIRECTIONAL_INDEXED_RECORD_MARGIN`
  (`0x100`, 0.0625 cells) otherwise. Each replacement component is the low
  32-bit product of `C` and the corresponding component of `N`, divided by
  `0x1000` (1.0) with rounding toward zero. On a second occurrence it exits
  without adding the replacement to position.
- If none of these cases applies, it exits without adding the displacement.

`VectorNormalS` reads three 32-bit input words and writes three output
halfwords. Its [ST_NormalizeVectorRegisters](../scr/st/ST_NormalizeVectorRegisters.c)
helper squares the signed low halfword of each input with the GTE and sums
the three results with 32-bit wrapping. It writes the sum to the GTE's
leading-bit input and clears the low bit of the resulting count. The GTE
count is the number of consecutive bits matching the sign bit, including the
sign bit. Let the resulting even count be `n`. The helper shifts the signed sum left by `n - 24` when
`n >= 24`, otherwise arithmetic-shifts it right by `24 - n`; the shift count
is masked to five bits. It subtracts 64, doubles the result, and uses it as
a byte offset from `ST_NORMALIZATION_INVERSE_ROOT_TABLE_ADDRESS`. The signed
table halfword and original signed low input halfwords feed a GTE GPF command.
Finally, it arithmetic-shifts each resulting MAC word right by
`((31 - n) >> 1) & 31` and `VectorNormalS` stores the low halfword.
The lookup and shifts are part of the calculation; the output is not an
exact mathematical division by a real-valued length. Three zero input
halfwords produce three zero output halfwords. With the supplied executable's
table, input `(0x1000, 0, 0x1000)` produces `(0xB50, 0, 0xB50)`:
each nonzero output component is 0.70703125 in Q.12.

On the first occurrence of any of the three correction cases, let `t` be
`ratan2(normalZ, normalX)` for a normal-threshold result, or
`ratan2(playerZ - referenceZ, playerX - referenceX)` for an object hit. Let
`p` be the low 32-bit product of the original input magnitude and
`rcos(inputAngle - 0x400 - t)`, divided by `0x1000` (1.0) toward zero. The
function stores the low halfword of `p` in
`ST_COLLISION_QUERY_CORRECTION_MAGNITUDE`. If `p` is nonzero, it adds
`rcos(t + 0x400) * p / 4096` to replacement X and
`rsin(t + 0x400) * p / 4096` to replacement Z, using low 32-bit products
and division toward zero, then queries again. If `p` is zero it queries again
without those additions. On exit it adds one quarter of signed
`ST_COLLISION_QUERY_RECORD_VECTOR_SUM_X` (`0x47CE`) and
`ST_COLLISION_QUERY_RECORD_VECTOR_SUM_Z` (`0x47D2`) to `ST_PLAYER_MOVE_X` and `ST_PLAYER_MOVE_Z`,
respectively, rounding toward zero, and stores the final X/Y/Z displacement
as `ST_PLAYER_COLLISION_DISPLACEMENT_X`, `ST_PLAYER_COLLISION_DISPLACEMENT_Y`,
and `ST_PLAYER_COLLISION_DISPLACEMENT_Z` (`0x260`, `0x262`, `0x264`). The query fields
and result bits are identified by their offsets and masks. **OPEN:**
their complete geometric meanings have not been established.

Both geometry record helpers first rotate the record vector and take its dot
product with a signed-halfword displacement prepared by
`ST_QueryLevelGeometry`. When the signed 32-bit sum is positive and neither
bit `0x1` nor bit `0x4` is set in `ST_GEOMETRY_RECORD_FLAGS`, the helper skips
that record. The prepared components are
`ST_COLLISION_GEOMETRY_SCRATCH_EXTENDED_DISPLACEMENT_X`,
`ST_COLLISION_GEOMETRY_SCRATCH_EXTENDED_DISPLACEMENT_Y`, and
`ST_COLLISION_GEOMETRY_SCRATCH_EXTENDED_DISPLACEMENT_Z`. The products and sum
wrap to 32 bits. For `ST_COLLISION_QUERY_DIRECTION_FROM_DISPLACEMENT`, used by
`ST_PlayerMoveWithCollision`, the prepared X/Z
components are the displacement plus a horizontal extension of
`ST_COLLISION_QUERY_HORIZONTAL_PARAMETER` in the displacement direction; its Y
component is the displacement Y plus `ST_COLLISION_QUERY_VERTICAL_PARAMETER`.
Each prepared component is stored as a halfword. If the horizontal displacement
is zero, the prepared X/Z components are zero. With an explicit query direction,
used by `ST_ApplyDirectionalMovementWithCollision`, X/Z use that direction for
the horizontal extension and Y is the displacement Y without the vertical
parameter added. Passing this test does not by itself select a record: distance
and edge tests follow.

For a three- or four-vertex record that reaches horizontal selection with bit
`0x2` set in `ST_GEOMETRY_RECORD_FLAGS`, the helper tests two points: the query's current
position, and a reference point with the same X/Z and Y equal to query Y plus
`ST_COLLISION_QUERY_VERTICAL_PARAMETER` minus
`ST_COLLISION_GEOMETRY_REFERENCE_Y_BIAS` (`0x403`, 0.250732421875 cells).
For each point it subtracts that point from each transformed vertex, takes the
signed low halfword of each component, and uses the GTE outer-product command
to form cross products in vertex order `(0,1)`, `(1,2)`, `(2,0)` for a
three-vertex record, or `(0,1)`, `(1,3)`, `(3,2)`, `(2,0)` for a four-vertex
record. The command arithmetic-shifts each cross-product component right by
12 before storing its 32-bit result. For each cross product, the helper takes
the dot product with the rotated record vector using 32-bit wrapped products
and sums. If all signed results are nonpositive, it considers that record
for the point's direct normal result. Otherwise it takes the dot products with
the signed extended displacement; if all are nonnegative, it considers
the record for the point's fallback result. Equality passes both comparisons.

The current-position test updates
`ST_COLLISION_GEOMETRY_SCRATCH_FIRST_NORMAL_DISTANCE` or
`ST_COLLISION_GEOMETRY_SCRATCH_FIRST_NORMAL_FALLBACK_DISTANCE`; the reference
point test updates `ST_COLLISION_GEOMETRY_SCRATCH_SECOND_NORMAL_DISTANCE` or
`ST_COLLISION_GEOMETRY_SCRATCH_SECOND_NORMAL_FALLBACK_DISTANCE`. Each update
keeps a strictly smaller signed point-to-record value and its rotated vector.
The current-position value is used for the first pair, and the reference-point
value for the second. Once all records have been scanned,
`ST_QueryLevelGeometry` uses a fallback value and its vector only if the
corresponding direct value remains at `ST_COLLISION_GEOMETRY_NO_RESULT`. The later
threshold comparisons produce the query's first and second normal result bits.
Earlier direction, distance, and record-flag checks can reject the record
before these tests. See
[ST_TestThreeVertexCollisionRecords](../scr/st/ST_TestThreeVertexCollisionRecords.c),
[ST_TestFourVertexCollisionRecords](../scr/st/ST_TestFourVertexCollisionRecords.c),
[ST_TransformCandidateVertices](../scr/st/ST_TransformCandidateVertices.c),
and [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c).

For example, take only stage 0 model 2's `0x29CC` record, with placement 3's
rotation and origin, and no matching active or indexed records. Its transformed
vector is `(0, 0, 0x1000)` (0.0, 0.0, 1.0) and its plane Z is `0x3F400`
(63.25 cells). With query X `0x40000` (64.0 cells), Z `0x3F500` (63.3125
cells), zero X/Y displacement, Z displacement `-0x30` (-0.01171875 cells),
and the normal movement query parameters, the record produces:

| Query Y (cells) | Result bit | Selected normal result (cells) |
| --- | --- | --- |
| `0x41480` (65.28125) | `ST_COLLISION_RESULT_SECOND_NORMAL_THRESHOLD` | `0x100` (0.0625) |
| `0x416C0` (65.421875) | `ST_COLLISION_RESULT_FIRST_NORMAL_THRESHOLD` | `0x100` (0.0625) |

In either case, starting `ST_PlayerMoveWithCollision` with those coordinates,
`ST_PLAYER_MOVE_Z = -0x30` (-0.01171875 cells), zero X and vertical movement,
and zero movement magnitude replaces the Z displacement with `0x1`
(0.000244140625 cells). The next query returns zero, and the call stores Z
`0x3F501` (63.312744140625 cells). With this record alone, the geometry Y
result is the no-result sentinel, so the subsequent vertical update sets
`ST_PLAYER_VERTICAL_MOVE` to `0x10` (0.00390625 cells per call). This example
does not establish which records are selected when other geometry is present.

For the same isolated record, X and Z, and with the player's stored movement
components zero, `ST_ApplyDirectionalMovementWithCollision` called with angle
`0xC00` (three quarters of a full turn) and magnitude `0x30` (0.01171875
cells) produces a different result at Y `0x416C0` (65.421875 cells): it applies
Z `-0x30`, ending at `0x3F4D0` (63.30078125 cells). At Y `0x41C00` (65.75
cells), it instead applies Z `0x1` (0.000244140625 cells), ending at `0x3F501`
(63.312744140625 cells). The two passes' different prepared Y components are
part of their geometry queries; the edge tests also determine the result.

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
pointer to a position and selects a reference in this order:

1. If `ST_COLLISION_QUERY_RESULT_FLAGS` contains `ST_COLLISION_RESULT_PLAYER_HIT`,
   select the player position and its fourth word. Set
   `ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_EXTENT` to `0x100` (0.0625 cells)
   and `ST_COLLISION_QUERY_REFERENCE_SECOND_VALUE` to `0x3E8` (1000).
2. Otherwise, if `ST_COLLISION_QUERY_ACTIVE_RECORD_INDEX` is not `-1`, select
   that active record's position and fourth word, and copy its horizontal and
   vertical extents to the two query values.
3. Otherwise, if `ST_COLLISION_QUERY_INDEXED_RECORD_INDEX` is not `-1`, select
   that level record's position and fourth word. Its
   `ST_LEVEL_RECORD_INDEXED_DATA_INDEX` selects an indexed record. Copy
   `ST_LEVEL_RECORD_INDEXED_HORIZONTAL_EXTENT` and
   `ST_LEVEL_RECORD_INDEXED_REFERENCE_SECOND_VALUE` to the two query values.
   If `ST_LEVEL_RECORD_REFERENCE_FLAGS` contains
   `ST_LEVEL_RECORD_FLAG_SCALE_REFERENCE_EXTENT`, multiply the horizontal
   extent by `ST_LEVEL_RECORD_REFERENCE_EXTENT_SCALE`, take the low 32-bit
   product, and arithmetic-shift it right by 7 before storing.
4. If both index words equal `-1` and the player-hit bit is clear, leave the
   reference fields unchanged.

For each selected reference, the helper copies the four words to
`ST_COLLISION_QUERY_REFERENCE_X`, `_Y`, `_Z`, and `_FOURTH_WORD`. It subtracts the selected X and Z words from
the supplied position's X and Z words, sums the low 32-bit squares with
32-bit wrap, passes the sum to `SquareRoot0`, and stores its result in
`ST_COLLISION_QUERY_REFERENCE_HORIZONTAL_DISTANCE`. The dispatcher writes
the two index fields from the
results of `ST_QueryActiveCollisionRecords` and `ST_QueryIndexedCollisionRecords`, respectively. **OPEN:** the
record types and the physical meaning of the second value and fourth word
remain unidentified.

### Indexed-record collision check

[ST_QueryIndexedCollisionRecords](../scr/st/ST_QueryIndexedCollisionRecords.c)
scans up to 240 records beginning at `ST_LEVEL_RECORD_TABLE_ADDRESS`, with a
stride of 88 bytes. For each record it reads the unsigned
`ST_LEVEL_RECORD_INDEXED_DATA_INDEX`. It skips the record if the index is
`0xFFFF`, if the record address equals the pointer at
`ST_LEVEL_RECORD_QUERY_EXCLUDED_ADDRESS`, or if
`ST_LEVEL_RECORD_INDEXED_HORIZONTAL_EXTENT` of the indexed 24-byte record in
`ST_LEVEL_RECORD_INDEXED_DATA_TABLE_ADDRESS` is zero.
[ST_UpdateLevelRecords](../scr/st/ST_UpdateLevelRecords.c) sets the pointer
at `ST_LEVEL_RECORD_QUERY_EXCLUDED_ADDRESS` to each nonempty record while
processing it and clears the pointer after its 240-record loop. A collision
query made during that processing skips the current record.

The routine subtracts `ST_COLLISION_QUERY_POSITION_X` (`0x04`) and
`ST_COLLISION_QUERY_POSITION_Z` (`0x0C`) from record X
and Z (`ST_LEVEL_RECORD_POSITION_X` and `ST_LEVEL_RECORD_POSITION_Z`) using
32-bit arithmetic. It skips the
record unless both differences lie in `[-0x2800, 0x2800]`. It calculates a
distance with `SquareRoot0` from the 32-bit wrapped sum of their squares,
and an angle with `ratan2` from those differences. It interprets the low
halfword of that angle as signed. From the distance it subtracts the low
32-bit product of signed `ST_COLLISION_QUERY_HORIZONTAL_MAGNITUDE` (`0x22`) and
`rcos(ST_COLLISION_QUERY_DIRECTION - angle)`, divided by 4096
with rounding toward zero. It skips the record unless this result is less
than the sum of unsigned `ST_COLLISION_QUERY_HORIZONTAL_PARAMETER` and the indexed record's
unsigned `ST_LEVEL_RECORD_INDEXED_HORIZONTAL_EXTENT`.

After a record passes that horizontal test, the function compares its Y
`ST_LEVEL_RECORD_POSITION_Y` to `ST_COLLISION_QUERY_POSITION_Y` (`0x08`). If record Y is numerically less, it returns `-1`
immediately when record Y is less than query Y minus unsigned query
`ST_COLLISION_QUERY_VERTICAL_PARAMETER`. Otherwise, it returns `-1` immediately when query Y is
less than record Y minus the record's unsigned `ST_LEVEL_RECORD_VERTICAL_EXTENT`. If neither
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
inclusive interval from player Y to `player Y + 1600 + ST_LEVEL_RECORD_VERTICAL_EXTENT`.
The horizontal comparison adds 256 to `ST_LEVEL_RECORD_INDEXED_HORIZONTAL_EXTENT`.
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
`ST_PlayerMoveWithCollision` when `ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED`
(`0x239`) is zero. If
the byte at `ST_ADJUST_WALK_SETTING_ADDRESS` (`0x801CD122`) is zero, the adjustment is cleared. Otherwise the routine adds
the unsigned `ST_PLAYER_MOVE_MAGNITUDE` to the unsigned
`ST_PLAYER_VIEW_OSCILLATION_PHASE` (`0x2D0`), masks the sum with `0xFFF`, and stores it as the new phase.
It computes `t = rsin(phase) >> 5` using an arithmetic shift, then stores
`abs(t) - (abs(t) >> 2)` as the adjustment. If the disable byte is
nonzero, this part of the routine leaves both phase and adjustment unchanged.
[ST_ResetPlayerViewAndVerticalMovement](../scr/st/ST_ResetPlayerViewAndVerticalMovement.c) clears the phase and both Y
adjustments. **OPEN:** the in-game meaning and producers of the disable byte
and the second adjustment have not been established.

[sub_8002B2E8](../scr/st/sub_8002B2E8.c) also reads `ST_ADJUST_WALK_SETTING_ADDRESS` as
the first of four consecutive bytes. For each byte it renders a pair of
text entries with their two styles exchanged when the byte equals 1. On the
supplied disc, the first setting is labeled **ADJUST WALK**, with ON and OFF
choices. Its movement effect is the view oscillation described above.

## Movement sound countdown

When `ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED` is zero and signed
`ST_PLAYER_VERTICAL_MOVE` is less than `0x100` (0.0625 cells per update),
`ST_PlayerMoveWithCollision` updates the signed 32-bit
`ST_PLAYER_MOVEMENT_SOUND_COUNTDOWN` (`0x244`). Let `m` be the signed
`ST_PLAYER_MOVE_MAGNITUDE` and `L` the signed movement limit. The countdown
decrement is `3 * m` if `m < (L >> 2)`, `(3 * m) / 2` rounded toward zero
if `m < (L >> 1)`, and `m` otherwise. The comparisons are evaluated in that
order and the shifts are arithmetic. The subtraction wraps to 32 bits. If
the signed countdown becomes negative, it is reset to `0x800` (0.5 in the
spatial-cell scale) and [ST_RequestMovementSound](../scr/st/ST_RequestMovementSound.c)
receives `m`. [sub_8003E874](../scr/st/sub_8003E874.c) also initializes the
countdown to `0x800` (0.5).

The sound helper reads `ST_COLLISION_QUERY_RECORD_FLAGS` from the shared
collision state. It requests sound ID `0x13` when any bit in `0x34` is set;
otherwise it requests `0x12` when the low two bits equal 2, or `0x11` for
the remaining values. Its parameter is `(m >> 2) + 0x40`, with an arithmetic
shift. [ST_QueryLevelGeometry](../scr/st/ST_QueryLevelGeometry.c) writes
the flag word from its collision-record scratch flags. **OPEN:** the names
of these sounds and the complete meaning and lifetime of the flags.

When candidates exist, `ST_QueryLevelGeometry` first clears
`ST_COLLISION_GEOMETRY_SCRATCH_RECORD_FLAGS`, lets the three- and
four-vertex tests update it, and copies it to `ST_COLLISION_QUERY_RECORD_FLAGS`.
When there are no candidates, the query returns before that copy, leaving the
previous record flags in place. `ST_ResetPlayerViewAndVerticalMovement` clears
the shared flags. On qualifying record paths, both geometry tests inspect the
record's flag halfword and `ST_COLLISION_QUERY_RECORD_FLAG_DISABLE_MASK`:
record bit `0x80` replaces the scratch flags with `0x8` when disable bit
`0x8` is clear; record bit `0x100` replaces them with `0x10` when disable
bit `0x10` is clear; record bit `0x200` ORs in `0x20` when disable bit
`0x20` is clear. The checks run in that order. Other record bits and earlier
tests can also change the scratch flags. The record flags' full meanings are
**OPEN**.

In [ST_UpdateLevelStreaming](../scr/st/ST_UpdateLevelStreaming.c), when
the signed status halfword at `ST_LEVEL_STREAMING_STATUS_ADDRESS` and
`ST_LEVEL_STREAMING_PHASE` both equal 1, the game queues a resource, clears
`ST_PLAYER_STREAMING_CLEAR_WORD`, sets `ST_PLAYER_STREAMING_RESET_HALFWORD`
to `-1`, and copies the level table. It then calls
[ST_TranslatePlayerPosition](../scr/st/ST_TranslatePlayerPosition.c) if
`ST_LEVEL_STREAMING_TRANSLATION_COARSE_X` is not
`ST_LEVEL_STREAMING_NO_TRANSLATION`. This adds offsets directly to player X,
Y, and Z. For each axis, the offset is `(coarse << 12) + (fine << 7)`, using
the matching `ST_LEVEL_STREAMING_TRANSLATION_COARSE_X`, `_Y`, or `_Z` and
`ST_LEVEL_STREAMING_TRANSLATION_FINE_X`, `_Y`, or `_Z`. Each byte is signed and
each position addition wraps to 32 bits. **OPEN:** the record's full meaning
and whether loaded code has additional request callers have not been established.

On its request path, [ST_RequestLevelStreaming](../scr/st/ST_RequestLevelStreaming.c)
sets the streaming status to 1, the phase to 0, and stores six bytes from
the caller's stack arguments in the streaming translation fields. If an
existing stream is active, `ST_LEVEL_STREAMING_TRANSLATION_APPLIED` is
nonzero, the requested primary ID is neither `0xFF` nor the previous
`ST_LEVEL_STREAMING_REQUEST_PRIMARY_ID`, and the supplied coarse X is
`ST_LEVEL_STREAMING_NO_TRANSLATION`, it instead negates each previous
coarse translation byte modulo 256. The fine bytes remain from the caller.
A separate path in
[ST_StartLevelStreamingAndWait](../scr/st/ST_StartLevelStreamingAndWait.c) sets status to 1 and phase to 0,
and writes `ST_LEVEL_STREAMING_NO_TRANSLATION` to the three coarse fields;
the X byte causes the later translation to be skipped.

On one path through [ST_UpdateLevelRecords](../scr/st/ST_UpdateLevelRecords.c), the six
arguments come from `ST_LEVEL_RECORD_TRANSLATION_COARSE_X`, `_Y`, and `_Z`
and `ST_LEVEL_RECORD_TRANSLATION_FINE_X`, `_Y`, and `_Z` in the current
level record. The resulting translation uses steps of 4096
and 128 stored position units, respectively. These are the same 88-byte
records scanned by `ST_QueryIndexedCollisionRecords`.

The other direct request calls in the recompiled source, in
[ST_UpdateLevelRecords](../scr/st/ST_UpdateLevelRecords.c),
[ST_RequestLevelStreamingPair](../scr/st/ST_RequestLevelStreamingPair.c), and
[ST_SetPlayerPositionAndRequestLevelStreaming](../scr/st/ST_SetPlayerPositionAndRequestLevelStreaming.c),
pass `ST_LEVEL_STREAMING_NO_TRANSLATION` for all three coarse values and zero
for all three fine values. The active-stream replacement described above can
still change those coarse values.

`ST_SetPlayerPositionAndRequestLevelStreaming` also writes player position
directly. For each axis it takes an unsigned grid byte and a signed fine
halfword from its request, and stores `(grid << 12) + fine` in the corresponding
`ST_PLAYER_POSITION_*` word. The request offsets are named in
[ST_PlayerPositionRequest.h](../scr/st/ST_PlayerPositionRequest.h). It also
copies `ST_PLAYER_POSITION_REQUEST_YAW` to `ST_PLAYER_YAW` and clears
`ST_PLAYER_PITCH`. It clears
the transient view rotations, movement limits and movement components, both
contribution vectors, and the collision-record vector sums. **OPEN:** the
complete format and in-game meaning of this request are not established.

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
[ST_EnterPlayerZeroHpState](../scr/st/ST_EnterPlayerZeroHpState.c) with zero and sets
`ST_PLAYER_HIGH_DOWNWARD_TRIGGERED` (`0x238`) to 1. Unless `ST_PLAYER_STATE`
(`0x218`) is already `ST_PLAYER_STATE_ZERO_HP` (4), that helper changes the
state to 4, clears `ST_PLAYER_CURRENT_HP`, and performs additional state
updates. [ST_AdjustPlayerHp](../scr/st/ST_AdjustPlayerHp.c) also calls the
helper when its signed 32-bit sum of current HP and the requested adjustment
is nonpositive; otherwise it caps the sum to maximum HP before storing it.
The [player-stat display](../scr/st/sub_80029B74.c)
reads `ST_PLAYER_CURRENT_HP`, `ST_PLAYER_MAX_HP`, `ST_PLAYER_CURRENT_MP`,
and `ST_PLAYER_MAX_MP`. [ST_ResetPlayerCountersAndState](../scr/st/ST_ResetPlayerCountersAndState.c)
clears the high-downward flag. **OPEN:** the full set of meanings of player
state values and all effects of the high-downward flag are not established.

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

With only one candidate placement containing that horizontal plane, the initial player position,
zero X/Z displacement, and the normal movement query dimensions, the
geometry routine produces the following `ST_COLLISION_QUERY_PROPOSED_Y_RESULT`
values. In each case `ST_COLLISION_QUERY_CURRENT_Y_RESULT` is `0x240`
(0.140625 cells), and the result mask is
`ST_COLLISION_RESULT_CURRENT_Y_THRESHOLD` (`0x1`).

| Proposed Y displacement | Proposed Y result |
| --- | --- |
| `0` | `0x240` (0.140625 cells) |
| `0x100` (0.0625 cells) | `0x140` (0.078125 cells) |
| `0x240` (0.140625 cells) | `0` |
| `-0x100` (-0.0625 cells) | `0x340` (0.203125 cells) |
| `0x340` (0.203125 cells) | `-0x100` (-0.0625 cells) |
| `0x341` (0.203369140625 cells) | `0x240` (0.140625 cells) |

At the final displacement the proposed-position value is below `-0x100`
(-0.0625 cells), so that record does not replace the proposed-position
sentinel; the query uses the current-position result instead.

With that same single plane, no matching active or indexed records, zero X/Z
movement, initial Y `0x419C0` (65.609375 cells), and initial vertical movement
zero, successive calls to `ST_PlayerMoveWithCollision` produce the following
Y and next vertical movement. Each call applies the previous vertical movement
before calculating the next one.

| Call | Player Y after call (cells) | Next vertical movement (cells per call) |
| --- | --- | --- |
| 1 | `0x419C0` (65.609375) | `-0x10` (-0.00390625) |
| 2 | `0x419B0` (65.60546875) | `-0x40` (-0.015625) |
| 3 | `0x41970` (65.58984375) | `-0x70` (-0.02734375) |
| 4 | `0x41900` (65.5625) | `-0xA0` (-0.0390625) |
| 5 | `0x41860` (65.5234375) | `-0xD0` (-0.05078125) |
| 6 | `0x41790` (65.47265625) | `-0x100` (-0.0625) |
| 7 | `0x41690` (65.41015625) | `-0xD0` (-0.05078125) |
| 8 | `0x415C0` (65.359375) | `0` (0.0) |

The seventh call limits the next movement to the remaining separation from
`q = 0x640` (0.390625 cells). On the eighth call that separation is reached
and the next vertical movement is zero. Under the same conditions, subsequent
calls leave Y and vertical movement unchanged.

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
update), and calls `ST_RequestSoundEffect` with ID `0x14` and parameter
`0x40 + (limited_result >> 3)`. The latter can reach `SsUtKeyOn` through
`ST_StartSoundEffect`. It also calls
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

When `d == 0`, vertical movement and `ST_PLAYER_VIEW_AND_SOUND_UPDATE_DISABLED` are
cleared. Other collision branches may change the applied displacement. These
are per-update stored integer values; the physical units have not been
established. With one spatial cell as one world unit, divide
positions and movement displacements by 4096 to obtain values with 12
fractional bits. See [STAGE_UNITS.md](STAGE_UNITS.md) for the extracted
geometry and the relationship between model, grid, and player coordinates.

Near the end of `ST_PlayerMoveWithCollision`,
[ST_UpdatePlayerCollisionYFlags](../scr/st/ST_UpdatePlayerCollisionYFlags.c)
compares `ST_COLLISION_QUERY_FIRST_PLAYER_Y_COMPARISON` (`0x4C`) and
`ST_COLLISION_QUERY_SECOND_PLAYER_Y_COMPARISON` (`0x58`) with `ST_PLAYER_POSITION_Y`,
using signed 32-bit comparisons. For each query word other than the sentinel
`0x80000`, it writes 1 to `ST_PLAYER_FIRST_COLLISION_Y_FLAG` (`0x2EC`) or
`ST_PLAYER_SECOND_COLLISION_Y_FLAG` (`0x2F0`), respectively, if the query value is less than player Y, or 0 otherwise. A sentinel
leaves that flag unchanged. If `ST_LEVEL_STREAMING_APPLIED_ID_FIRST` is 10 and signed
`ST_PLAYER_STREAMING_RESET_HALFWORD` is `0x147`, the function clears `ST_PLAYER_SECOND_COLLISION_Y_FLAG` even if the second
query word is the sentinel. The game state machine reads this flag when
choosing subsequent behavior. **OPEN:** the physical meanings of these flags
and the special-case values have not been established.
