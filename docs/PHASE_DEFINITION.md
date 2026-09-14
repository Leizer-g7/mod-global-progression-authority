# GPA Phase Definition

## 1. Purpose

GPA separates global progression policy from integration-specific policy.

The authoritative global state is:

    ProgressionState
        phase
        contentStage

A PhaseDefinition describes only progression policy that is globally meaningful
to the server.

It must not become a container for implementation details belonging to
Playerbots, AH Bot Plus, LFG, Battlegrounds, or other optional integrations.

---

## 2. Core PhaseDefinition

The conceptual GPA Core definition is:

    PhaseDefinition
        phase

        contentStageMin
        contentStageMax

        playerMaxLevel

        profession policy
            craftingCap
            gatheringCap
            secondaryCap
            ridingCap

        world availability
            outlandEnabled
            northrendEnabled

        deathKnightEnabled

The exact C++ layout may use nested structures, but these ownership boundaries
are part of the GPA architecture.

---

## 3. Core fields

### phase

Stable identifier for the global phase.

Initial migration values:

    20
    30
    40
    50
    60
    70
    80

The architecture must not assume that only these values can ever exist.

---

### contentStageMin / contentStageMax

Defines which historical ContentStage values are valid while the phase is
active.

These fields belong to GPA Core because they define validity of the
authoritative ProgressionState itself.

Initial migration ranges:

    Phase 20 -> 0..0
    Phase 30 -> 0..0
    Phase 40 -> 0..0
    Phase 50 -> 0..0
    Phase 60 -> 0..8
    Phase 70 -> 8..13
    Phase 80 -> 13..18

The shared boundary stages are intentional.

For example:

    Phase 60 / ContentStage 8

and:

    Phase 70 / ContentStage 8

are both valid states.

This allows expansion transitions without inventing a second historical
progression authority.

---

### playerMaxLevel

Global player level capability for the phase.

Initial migration values:

    Phase 20 -> 20
    Phase 30 -> 30
    Phase 40 -> 40
    Phase 50 -> 50
    Phase 60 -> 60
    Phase 70 -> 70
    Phase 80 -> 80

Player enforcement belongs to PlayerAdapter.

The value itself belongs to global phase policy.

---

## 4. Profession policy

Profession limits are global progression policy.

Initial migration values:

             Crafting   Gathering   Secondary   Riding

    Phase20     150         150         150        75
    Phase30     225         225         225        75
    Phase40     300         300         300       150
    Phase50     300         300         300       150
    Phase60     300         300         300       150
    Phase70     375         375         375       300
    Phase80     450         450         450       300

PhaseDefinition owns these limits.

ProfessionAdapter owns the mechanics used to enforce them:

    max skill exposure
    skill clamping
    trainer spell filtering
    rank/step validation

GPA Core must not know spell IDs, skill spell wrappers, trainer internals, or
AzerothCore trainer implementation details.

---

## 5. World availability

The following are global world-progression capabilities:

    outlandEnabled
    northrendEnabled

Initial migration policy:

              Outland   Northrend

    Phase20     NO          NO
    Phase30     NO          NO
    Phase40     NO          NO
    Phase50     NO          NO
    Phase60     NO          NO
    Phase70     YES         NO
    Phase80     YES         YES

The availability values belong to PhaseDefinition.

TravelAdapter owns enforcement details.

Examples of TravelAdapter responsibilities:

    Outland direct-travel minimum level
    Northrend direct-travel minimum level
    expansion instance parent resolution
    TBC starter-zone exceptions
    Naxxramas map 533 exception
    login relocation
    teleport denial messages

Those details must not enter GPA Core.

---

## 6. Death Knight availability

Death Knight availability is global phase policy.

Initial migration policy:

    Phase20 -> disabled
    Phase30 -> disabled
    Phase40 -> disabled
    Phase50 -> disabled
    Phase60 -> disabled
    Phase70 -> disabled
    Phase80 -> enabled

This is not a Playerbots-only setting.

Player policy and Playerbots policy may both consume the same global value.

The Playerbots adapter translates it into Playerbots configuration such as
DisableDeathKnightLogin.

The Core PhaseDefinition does not know that Playerbots field.

---

## 7. Playerbots phase policy

The following V1 fields do NOT belong to GPA Core:

    randomBotMaxLevel
    botGearMaxItemLevel
    botDistribution

They become PlayerbotsAdapter phase policy.

Initial migration baseline:

    Phase20
        MaxLevel = 20
        GearMaxItemLevel = 25
        Distribution = 1-9:15,10-19:50,20:35

    Phase30
        MaxLevel = 30
        GearMaxItemLevel = 35
        Distribution = 1-9:10,10-19:15,20-29:45,30:30

    Phase40
        MaxLevel = 40
        GearMaxItemLevel = 45
        Distribution = 1-9:8,10-19:10,20-29:15,30-39:42,40:25

    Phase50
        MaxLevel = 50
        GearMaxItemLevel = 55
        Distribution = 1-9:6,10-19:8,20-29:10,30-39:12,40-49:40,50:24

    Phase60
        MaxLevel = 60
        GearMaxItemLevel = 78
        Distribution = 1-9:5,10-19:7,20-29:8,30-39:10,40-49:12,50-59:38,60:20

    Phase70
        MaxLevel = 70
        GearMaxItemLevel = 125
        Distribution = 1-9:4,10-19:5,20-29:6,30-39:7,40-49:8,50-59:10,60-69:40,70:20

    Phase80
        MaxLevel = 80
        GearMaxItemLevel = 224
        Distribution = 1-9:3,10-19:4,20-29:5,30-39:6,40-49:7,50-59:8,60-69:10,70-79:37,80:20

Although Playerbots MaxLevel currently equals playerMaxLevel in every V1 phase,
GPA will preserve it as an explicit Playerbots policy during parity work.

It must not initially be replaced by an implicit derivation.

After V1 parity is certified, simplification may be evaluated separately.

---

## 8. Playerbots implementation details

These values are adapter implementation/configuration details and must not be
stored in Core PhaseDefinition:

    TeleToShattrathCityWeight
    TeleToDalaranWeight

    LevelBrackets.Enabled
    bracket check frequencies
    flagged process limits
    faction synchronization

    RandomGearScoreLimit
    AutoGearScoreLimit

    DisableDeathKnightLogin

    SmartScale max level

GPA provides desired policy.

PlayerbotsAdapter translates that policy into the current Playerbots API.

---

## 9. Economy phase policy

The V1 field:

    auctionMaterialMaxItemLevel

does not belong to GPA Core PhaseDefinition.

It becomes progression-related economy policy consumed by an economy/AHBot
adapter.

Initial migration baseline:

    Phase20 -> 20
    Phase30 -> 30
    Phase40 -> 40
    Phase50 -> 50
    Phase60 -> 55
    Phase70 -> 70
    Phase80 -> 999

This value must not be confused with Playerbots gear item-level limits.

The two policies have different meanings.

---

## 10. Battleground policy

Battleground bracket selection is not Core PhaseDefinition data.

V1 currently derives a battleground plan from a hard-coded switch on phase.

GPA must move this mapping into BattlegroundAdapter policy data.

The adapter may use Phase as the key, but GPA Core must not know:

    WSG bracket indexes
    AB bracket indexes
    AV bracket indexes
    Eye of the Storm bracket indexes
    Isle of Conquest bracket indexes
    Playerbots BG count fields

The existing V1 behavior remains the parity baseline.

---

## 11. LFG policy

The following are adapter/global-feature configuration, not PhaseDefinition:

    LFG enabled for random bots
    Random Dungeon Finder enabled

GPA Core does not know Playerbots RandomBotJoinLfg or AzerothCore LFG option
bit masks.

LfgAdapter owns translation into those runtime systems.

---

## 12. Manager configuration

The following settings belong to ProgressionMgr configuration and are not
properties of a phase:

    GPA enabled
    default Phase
    default ContentStage
    rollback permission
    GM bypass

The persisted ProgressionState overrides defaults after a valid state exists.

Defaults exist to initialize a missing state, not to silently replace an
invalid persisted state.

---

## 13. State validation

A state is valid only when:

    the phase exists

and:

    contentStageMin <= ContentStage <= contentStageMax

Validation operates on the complete target ProgressionState.

GPA must not independently mutate Phase and ContentStage through unrelated
write paths.

Conceptually:

    RequestTransition(
        ProgressionState target
    )

rather than:

    SetPhase(...)
    SetContentStage(...)

as independent authorities.

---

## 14. Invalid persisted state

V1 may repair some invalid persisted values by selecting configured defaults or
the minimum ContentStage for a phase.

GPA intentionally changes this behavior.

If no persistent row exists:

    initialize using validated defaults

If a persistent row exists but is invalid:

    report an explicit startup error
    do not silently invent a different authoritative progression state

Any recovery operation must be deliberate and observable.

---

## 15. Phase profile separation

GPA conceptually has several policy layers keyed by the same Phase:

    Core PhaseDefinition
        global progression semantics

    PlayerbotsPhasePolicy
        Playerbots-specific desired state

    EconomyPhasePolicy
        progression-related economy desired state

    BattlegroundPhasePolicy
        BG automation desired state

    other future adapter policies

These policies may eventually be loaded from one physical GPA configuration
file.

Physical configuration location does not imply architectural ownership.

---

## 16. V1 parity rule

The first GPA implementation must reproduce the certified V1 behavior before
new progression features are introduced.

Architectural relocation of a field must not silently change its current
production semantics.

For parity:

    same valid Phase values
    same ContentStage ranges
    same player caps
    same profession caps
    same expansion availability
    same DK availability
    same Playerbots caps
    same Playerbots gear limits
    same Playerbots distributions
    same economy material limits
    same LFG behavior
    same Battleground behavior

Only after parity has been validated may these policies be intentionally
redesigned.
