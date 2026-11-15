# Player movement

The game state machine in `sub_80042D74.c` calls `ST_PlayerUpdateTurning`,
`ST_PlayerUpdateMovement`, and `ST_PlayerMoveWithCollision` in that order in
movement states. The code is recompiled MIPS: `ctx` registers and `MEM_*`
accesses represent emulated state, and comments retain original instructions.
`ST_PlayerState.h` names confirmed offsets from the player-state base
`0x80198F28`; it does not define a native C struct.

## Horizontal input

`ST_PLAYER_LOCAL_MOVE_X` (`0x2B8`) and `ST_PLAYER_LOCAL_MOVE_Z` (`0x2C0`)
are signed player-local sideways and forward/backward accumulators. The
control masks are initialized by `ST_InitDefaultControlBindings`: `0x10`
increases Z and `0x40` decreases Z; `0x800` increases X and `0x400`
decreases X. The bindings may be changed. The two opposing X controls
cancel; the positive Z branch takes precedence if both Z controls are held.

With a positive movement limit `L`, a held direction changes its accumulator
by `L >> 2`, clamped to `+L` or `-L`. With no direction it approaches zero
by `L >> 3`, clamped at zero. Reversing direction uses the held-direction
step. The gameplay state normally sets L to 192, but can reduce or clear it.
For example, starting from zero with only positive Z held and L unchanged,
Z is 48, 96, 144, 192, then 192 on successive updates. On release from 192,
it decreases by 24 per update. Integer shifts and state changes affect the
exact sequence.

The routine processes the X/Z magnitudes and rotates the local vector by
negative player yaw using the GTE. It also damps existing movement components
and adds other contributions. `ST_PlayerMoveWithCollision` tests the resulting
movement against the environment before applying position changes. The
accumulators therefore are not directly world-position increments.

## Collision query and response

`ST_PlayerMoveWithCollision` builds a shared query at `0x801B5948` from the
player position and proposed X/Y/Z displacement. It supplies `0x100` and
`0x640` as query dimensions or offsets and enables checks with flags `0x432`.
The exact collision shape and the meaning of both dimensions are not yet
established. `ST_RunCollisionQuery` dispatches the enabled checks; among
them, `ST_QueryLevelGeometry` searches level geometry. The query returns a bitmask and
stores additional collision data in the shared query structure.

When the result is zero, the current candidate displacement is applied to
position. Bit `0x1` also applies it, after which other bits are still handled.
Bits `0x2` and `0x8` select separate horizontal correction paths. These use
query data and the original movement direction to form another X/Z candidate,
which is queried again. The angle and perpendicular-direction calculations
are consistent with sliding along surfaces; the precise meaning of the result
bits and query fields is not fully established. The routine tracks each of
these two correction cases and applies the current candidate if that same
case is encountered again.

## Vertical movement

World Y increases downward: negative Y is up. `ST_PLAYER_VERTICAL_MOVE`
(`0x2D4`) is a signed 16-bit value used as the Y component of collision-tested
movement, so a positive value moves the player down. The movement routine applies the
current value before updating it for the next movement update. After the
collision query, let `q` be its word at `0x801B5974` and `d = q - 0x640`
(1600). When the geometry search finds no candidates, `ST_QueryLevelGeometry` sets q
to `0x80000`. The resulting positive d starts positive vertical movement and
then accelerates it, consistent with falling in empty space. The initial
player Y is also initialized from a decimal text field shifted by 12 bits,
less `0x640`, while the collision query
temporarily adds that same offset to Y. These independent paths support
negative Y being up. The exact meaning of q and the offset's relationship to
the player's full height are not established. The branches below describe
stored values.

For `d < 0`:

- If vertical movement is positive, call `sub_80057B48`, then arithmetic-shift
  the resulting signed value right by 2.
- If it is zero and `-31 <= d < 0`, set it to `-d`; if `d <= -32`, set it
  to -16. In the latter case clear the movement limit when `d >= -1024`.
- If it is negative, subtract 48, then clamp the result to no less than `d`.
  Clear the movement limit when `d >= -1024`.

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
are per-update integer values; the update frequency and physical units have
not been established. Position initialization uses a 12-bit shift on decimal
text fields, but that alone does not establish a fixed-point format for the
input accumulators.
