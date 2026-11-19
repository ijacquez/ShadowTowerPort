# ShadowTowerPort

This file is the constitution for ShadowTowerPort ("Project"). Read it at the
start of every session before doing any work.

---

## ⛔ Constitution.

This Project is being REVERSE ENGINEERED.

- This supersedes any pressure to ship fast. Completeness is absolute here.
- No documentation or source tree changes are accepted if not fully backed up by
  direct evidence.
- Game documentation describes the source tree and verified game behavior.
  Do not include investigation narratives, runtime observations, debugger
  transcripts, or reproduction and test procedures in game documentation.
- If a finding can't be resolved statically, record it as **OPEN** with the
  evidence gathered so far.
- Unknown is acceptable. Guessing is not.
  If something is unknown, STOP and document the unknown. No theories to fill in
  gaps.
- Symbol (read: functions, variables, etc.) and file rename are to be human readable
- Partial milestones are not accepted.
  Phase completion requires the user-visible end state, not "I think it should
  work now". Phase X is "player movement is fully documented and source tree is
  updated to reflect human readable functions and variables". Not "player input
  is found and two accumulators are used to compute the position".
- Broken tooling is never acceptable.
  Fix it when identified.
- Source code for tools must always be clean; readable and documented in a terse
  and concise manner.
- Comments in the source tree is welcomed.
  All must be up to date, concise, and to the point. They are to be markers to
  help you the agent pick up where you left off.

## Session start checklist

At the start of every session, before any code change:

1. Read this entire file: `AGENTS.md` and adhere to all rules.
2. This list will grow as the Project advances.
3. Documentation goes into `docs/` and needs to be always correct and up to date.
4. Use `scratchpad/` for your temporary work.
5. Tools are to always be written in Java.
6. You are allowed to modify the source tree (or duplicate then modify) if it
   facilitates the investigation.
7. Tests/tools that need to run for a considerable amount of time is allowed.

If any of these fail, do not proceed with the user's task — surface
the failure first.

## Skills

### Run under Xvfb

From the project root, build the game and link the disc image into the
scratch directory. The game expects the image to be named `Shadow_Tower.bin`
in its working directory.

```sh
make -j4
mkdir -p scratchpad
ln -sfn ../Shadow_Tower scratchpad/Shadow_Tower
ln -sfn ../Shadow_Tower.bin scratchpad/Shadow_Tower.bin
cd scratchpad
ALSOFT_DRIVERS=null LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a ./Shadow_Tower
```

This uses software rendering and disables audio output. Logs and other
runtime files are written in `scratchpad/`.
