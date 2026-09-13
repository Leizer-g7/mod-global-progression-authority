# GPA Core Contract

## 1. Authority

Global Progression Authority is the only writer of the global progression state.

The authoritative state contains:

    Phase
    ContentStage

No adapter or external module may independently change those values.

External systems may consume them.

Administrative commands may request a transition, but the request is always
validated and executed by GPA Core.

---

## 2. Progression State

The minimal authoritative state is conceptually:

    ProgressionState
        phase
        contentStage

The current state must always correspond to a valid PhaseDefinition.

An invalid Phase / ContentStage combination must never become authoritative.

Example:

    Phase 20 / ContentStage 0
        valid

    Phase 20 / ContentStage 1
        invalid if Phase 20 permits only stage 0

---

## 3. Phase Definition

A PhaseDefinition describes server policy for one global phase.

Initial policy fields expected from the existing V1 design include:

    phase

    playerMaxLevel

    botMaxLevel
    botGearMaxItemLevel

    craftingProfessionCap
    gatheringProfessionCap
    secondaryProfessionCap

    auctionMaterialMaxItemLevel

    allowOutland
    allowNorthrend

    allowDeathKnight

    minContentStage
    maxContentStage

Additional policy may be added later, but PhaseDefinition remains a data model,
not an integration implementation.

For example:

    PhaseDefinition

does NOT directly modify Playerbots.

It only states:

    botMaxLevel = 30

The Playerbots adapter is responsible for translating that policy.

---

## 4. Read Contract

Reading progression state must be safe and side-effect free.

Consumers must be able to query at least:

    IsEnabled
    ActivePhase
    ActiveContentStage
    ActiveState
    ActivePhaseDefinition

Policy-specific read helpers may exist where they provide a stable abstraction,
for example:

    GetPlayerMaxLevel
    GetProfessionCap
    IsOutlandAllowed
    IsNorthrendAllowed
    IsDeathKnightAllowed

Readers never obtain write authority.

---

## 5. Write Contract

Only GPA Core may perform a progression transition.

A requested transition contains a complete target state:

    old state
        ->
    target state

Examples:

    20 / 0 -> 30 / 0

    60 / 2 -> 60 / 3

    60 / 3 -> 60 / 2

Both forward progression and rollback use the same transition mechanism.

There must not be separate uncontrolled write paths for:

    phase
    content stage
    adapter-specific state

---

## 6. Validation

Before a target can become authoritative GPA must validate:

    GPA is enabled

    target phase exists

    target ContentStage is within the phase's valid range

    target state is internally consistent

    critical integration preconditions are satisfied

A rejected transition must leave the current state unchanged.

---

## 7. Transition Lifecycle

The conceptual lifecycle is:

    request
        |
        v
    validate target
        |
        v
    adapter preflight
        |
        v
    persist target
        |
        v
    publish new authoritative state
        |
        v
    apply adapters
        |
        v
    transition complete

If a critical adapter cannot safely apply the new policy, GPA must attempt to
restore the previous authoritative state and reapply the previous policy.

Optional adapter failure must not allow that adapter to become an independent
authority.

The implementation details of transaction and rollback behavior will be
validated before production migration.

---

## 8. Persistence

Persistent state must have one canonical source.

The existing V1 state model is the migration reference:

    active_phase
    active_content_stage

GPA must not maintain separate competing progression state in:

    Playerbots
    Individual Progression
    AH Bot Plus
    character-local progression writers

Compatibility mirrors may exist when an external module requires them, but they
are never authoritative.

---

## 9. Startup

On startup GPA must:

    load configuration
    load phase definitions
    load persisted state
    validate persisted state
    publish authoritative state
    initialize adapters
    apply current policy

An invalid persisted state must not silently fall back to an unrelated phase.

Startup behavior for invalid state must be explicit and logged.

---

## 10. Reload

Configuration reload must not silently change progression state.

A reload may update definitions and adapter configuration, but the currently
active state must first be revalidated against the new definitions.

If the active state is no longer valid, GPA must report the error rather than
inventing another progression state.

---

## 11. Rollback

Rollback is not a special implementation.

It is a normal transition from the current state to an earlier valid state.

Example:

    60 / 5
        ->
    60 / 4

or:

    70 / 8
        ->
    60 / 8

if that combination is valid.

Adapters must therefore support both increasing and decreasing progression
policy wherever technically possible.

Irreversible data mutations must be avoided.

---

## 12. Logging

Every accepted transition must log:

    previous Phase
    previous ContentStage

    target Phase
    target ContentStage

    validation result

    persistence result

    adapter results

Every rejected transition must explain why it was rejected.

Startup must log the complete authoritative state and important derived policy.
