# GPA State Model

## Phase and ContentStage are separate axes

Phase represents broad server capability.

ContentStage represents historical/endgame progression within that capability.

Initial migration model:

    Phase 20
        ContentStage 0

    Phase 30
        ContentStage 0

    Phase 40
        ContentStage 0

    Phase 50
        ContentStage 0

    Phase 60
        ContentStage 0..8

    Phase 70
        ContentStage 8..13

    Phase 80
        ContentStage 13..18

These ranges come from the currently validated V1 progression model and are the
initial GPA migration baseline.

They are configuration/policy data, not hard-coded architectural assumptions.

## Examples

Early leveling:

    Phase 30
    ContentStage 0

means roughly:

    global player capability is level 30-era
    historical endgame ContentStage remains at the starting stage

Vanilla endgame progression:

    Phase 60 / ContentStage 0
    Phase 60 / ContentStage 1
    ...
    Phase 60 / ContentStage 8

The level cap remains 60 while historical content progresses.

Expansion transition:

    Phase 70 / ContentStage 8

moves the broad server capability into TBC-era leveling while preserving the
historical handoff stage.

## Authority

Only GPA stores the authoritative Phase / ContentStage combination.

Per-character markers required for compatibility are mirrors.

Playerbots configuration values are derived policy.

AH candidate pools are derived policy.

Profession caps are derived policy.

None of those become additional progression authorities.
