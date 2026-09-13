# GPA Adapter Contract

## 1. Purpose

Adapters translate GPA policy into an external subsystem.

Examples:

    PlayerAdapter
    ProfessionAdapter
    PlayerbotsAdapter
    IndividualProgressionAdapter
    AuctionHouseBotAdapter
    TravelAdapter
    LfgAdapter
    BattlegroundAdapter

Adapters never own global progression state.

---

## 2. Dependency Boundary

GPA Core must not include implementation headers from:

    mod-playerbots
    mod-individual-progression
    mod-ah-bot-plus

Only the corresponding adapter may depend on an external module API.

Example:

    GPA Core
        |
        | ProgressionState / PhaseDefinition
        v
    PlayerbotsAdapter
        |
        v
    Playerbots API

A change in Playerbots should therefore affect the Playerbots adapter rather
than GPA Core.

---

## 3. Adapter Identity

Every adapter must have a stable diagnostic name.

Examples:

    Players
    Professions
    Playerbots
    IndividualProgression
    AuctionHouseBot
    Travel
    LFG
    Battlegrounds

The name is used in startup and transition diagnostics.

---

## 4. Availability

An optional integration must be able to report:

    available
    unavailable

An unavailable optional module must not stop GPA Core.

Example:

    AH Bot Plus not installed

must result in:

    AuctionHouseBot adapter: unavailable

rather than preventing GPA from starting.

The concrete compile-time/runtime module detection mechanism is intentionally
not frozen yet and will be selected after validating AzerothCore module build
behavior.

---

## 5. Preflight

Before a transition becomes authoritative, adapters that require validation may
inspect:

    current state
    target state
    current PhaseDefinition
    target PhaseDefinition

Preflight must not perform destructive changes.

It answers whether the adapter can safely support the target policy.

---

## 6. Apply

Apply receives the authoritative transition and translates it into subsystem
policy.

Conceptually:

    Apply(oldState, newState, newDefinition)

Examples:

PlayerbotsAdapter:

    random bot max level
    level distribution
    gear limits
    travel
    DK policy
    LFG
    battleground policy

ProfessionAdapter:

    profession skill caps
    trainer eligibility policy

IndividualProgressionAdapter:

    external ContentStage authority
    compatibility synchronization when necessary

AuctionHouseBotAdapter:

    item eligibility policy
    progression-related economic restrictions

---

## 7. Idempotency

Apply must be idempotent.

Calling Apply twice with the same state must not:

    duplicate database data
    duplicate rewards
    increase counters
    accumulate multipliers
    produce different policy

Adapters should prefer setting desired state rather than performing incremental
mutations.

Good:

    MaxLevel = 30

Bad:

    MaxLevel += 10

---

## 8. Rollback

Adapters must support application of an earlier valid GPA state whenever the
underlying subsystem permits it.

The preferred model is:

    Apply(previousState)

rather than implementing a completely separate rollback code path.

If an integration contains a genuinely irreversible action it must be clearly
classified and may not be executed as part of an ordinary GPA phase change
without an explicit migration design.

---

## 9. Critical vs Optional

Adapters may eventually be classified as:

    critical
    optional

Critical means failure can invalidate a requested GPA transition.

Optional means GPA may remain authoritative while the integration is marked
degraded and an explicit error is logged.

This classification will be frozen per adapter before implementation.

---

## 10. Configuration

Adapter-specific configuration belongs with the adapter.

GPA Core should not contain Playerbots-specific or AHBot-specific configuration
keys.

Core configuration should contain only progression-authority concerns.

---

## 11. External Patches

If an external module lacks a necessary extension point, the preferred patch is
a generic API.

Example:

    CanExternalPolicyListItem(ItemTemplate const*)

is preferable to:

    if (sGlobalProgressionAuthority.GetActivePhase() == ...)

inside AH Bot Plus.

The external module should know that an external policy provider exists, not
what GPA phases mean.

---

## 12. Diagnostics

Each adapter must eventually report:

    name
    availability
    enabled/disabled
    last applied state
    healthy/degraded status

The goal is for GPA status output to make integration problems obvious without
reading source code.
