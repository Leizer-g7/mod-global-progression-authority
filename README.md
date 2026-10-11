**Global Progression Authority (GPA)** is a development-stage global progression module for **AzerothCore 3.3.5a**. It provides one authoritative server-wide progression state consisting of a **Phase** and a **ContentStage**. Administrators advance progression explicitly; GPA applies policies and exposes the state to supported integrations.

> **Status:** `v0.1.0` is an initial development snapshot, **not a certified production release**. The current `main` branch may contain documentation and development changes beyond that tag. Compatibility with a fresh AzerothCore installation has not yet been certified.

## Features

- Persistent, validated global `Phase` / `ContentStage` state in the world database.
- Administrative status inspection and progression transitions.
- Player level-cap, profession/riding, travel/expansion and Death Knight policies.
- Integration adapters for Playerbots, Individual Progression and AH Bot Plus.
- Configurable GM bypass and rollback policy.

An adapter's presence does not guarantee compatibility with every version of an external module or full control of its behavior.

## Progression phases

| Phase | ContentStage range | Player level cap | Outland | Northrend | Death Knight |
| --- | --- | --- | --- | --- | --- |
| 20 | 0 | 20 | Locked | Locked | Locked |
| 30 | 0 | 30 | Locked | Locked | Locked |
| 40 | 0 | 40 | Locked | Locked | Locked |
| 50 | 0 | 50 | Locked | Locked | Locked |
| 60 | 0–8 | 60 | Locked | Locked | Locked |
| 70 | 8–13 | 70 | Available | Locked | Locked |
| 80 | 13–18 | 80 | Available | Available | Available |

These are the policy definitions in `src/PhaseCatalog.cpp`. `ContentStage` controls finer progression within a phase; it is not another character-level cap.

## Configuration

Configuration template: `conf/mod_global_progression_authority.conf.dist`.

```ini
GlobalProgression.Enable = 0
GlobalProgression.GMBypass = 1
GlobalProgression.DefaultPhase = 20
GlobalProgression.DefaultContentStage = 0
GlobalProgression.AllowRollback = 0
```

GPA is **disabled by default**. The default state is used only when the canonical persisted row is absent. `GMBypass` allows Game Masters to bypass supported player, profession and travel restrictions; it is not a substitute for progression administration. Rollbacks are disabled by default.

**Never run GPA and `mod-phase-progression` as simultaneous writers of global progression state.** Plan and test any migration before enabling GPA.

## Administration

| Command | Minimum security | Purpose |
| --- | --- | --- |
| `.gpa status` | Game Master | Show current progression status |
| `.gpa set <phase> <contentStage>` | Administrator | Request a validated transition |

Both commands support the console. Examples:

```text
.gpa status
.gpa set 30 0
.gpa set 60 0
.gpa set 70 8
.gpa set 80 13
```

Only valid phase/stage combinations are accepted, and the configured rollback policy applies.

## Build and installation status

The module was detected by the AzerothCore native module build system with `MODULES=static` in the development environment. The generated script loader registers `Addmod_global_progression_authorityScripts()`. A module-local `CMakeLists.txt` was not needed in that tested configuration.

For an **isolated development installation**:

1. Review the target AzerothCore revision and the compatibility requirements below.
2. Clone this repository into the AzerothCore `modules/mod-global-progression-authority` directory.
3. Configure AzerothCore with modules enabled in static mode, then compile using the target core's documented build procedure.
4. Apply or verify the GPA world-database schema from `data/sql/db-world/updates/2026_09_15_00_global_progression_state.sql` using the target installation's normal migration process. Do not manually apply it to production without a tested backup and migration plan.
5. Deploy the module configuration template through the target AzerothCore configuration workflow, keeping GPA disabled until validation is complete.
6. Confirm module registration, state persistence, commands and supported policies against isolated databases before considering activation.

**This is a development outline, not a certified copy-and-paste production installation procedure.** Existing local builds do not prove portability to all upstream revisions.

## Compatibility and integrations

- **AzerothCore:** some player quest-reward and trainer-filtering behavior relies on generic core extension hooks that were not available in the audited upstream revision.
- **Playerbots:** the adapter consumes supported settings/APIs; GPA does not independently implement Playerbots AI, XP progression, equipment or all gameplay systems.
- **Individual Progression:** global ContentStage integration depends on an external progression-provider extension point in the audited custom module.
- **AH Bot Plus:** phase-aware item eligibility depends on an appropriate extension point; pricing and other auction-economy features remain outside GPA Core.

The compatibility baseline in `docs/COMPATIBILITY.md` is dated **2026-09-13** and should be revalidated for newer revisions. Do not assume all historical requirements are unchanged.

## Migration and safety

Production replacement requires functional parity, adapter compatibility review, an isolated build, cloned-database smoke tests, backups and certification. Do not update external modules directly in production without this validation. See `docs/MIGRATION.md`.

## Technical documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Phase definitions](docs/PHASE_DEFINITION.md)
- [State model](docs/STATE_MODEL.md)
- [Persistence schema](docs/PERSISTENCE_SCHEMA.md)
- [Persistent transitions](docs/PERSISTENT_TRANSITIONS.md)
- [Core contract](docs/CORE_CONTRACT.md)
- [Adapter contract](docs/ADAPTER_CONTRACT.md)
- [Integrations](docs/INTEGRATIONS.md)
- [Compatibility](docs/COMPATIBILITY.md)
- [Migration strategy](docs/MIGRATION.md)

## License

License and third-party code provenance review are pending. **No license grant is implied by repository availability.** Complete the review before making this repository public or distributing it to others.
README
