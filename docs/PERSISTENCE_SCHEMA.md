# GPA Persistence Schema

## Decision

GPA reuses the existing World Database table:

    phase_progression_state

Canonical GPA state is stored in row:

    id = 1

The persisted authority is the complete pair:

    { active_phase, active_content_stage }

GPA does not create a second progression-state table.

## Schema

The required schema is:

    id                    TINYINT UNSIGNED PRIMARY KEY
    active_phase          TINYINT UNSIGNED NOT NULL
    active_content_stage  TINYINT UNSIGNED NOT NULL
    updated_at            TIMESTAMP

The GPA schema update is:

    data/sql/db-world/updates/
    2026_09_15_00_global_progression_state.sql

## Schema provisioning

Schema provisioning is performed before GPA runtime starts.

The WorldDatabase persistence adapter does not create or alter
database schema at runtime.

For a new installation the GPA SQL creates the table but does not
insert the canonical row.

The table therefore remains empty until ProgressionBootstrap
receives Missing, validates the configured default state, and
persists that complete default through IProgressionStateStore.

For an old V1 schema that already has active_phase but does not
have active_content_stage, the schema update adds only the missing
column:

    active_content_stage TINYINT UNSIGNED NOT NULL DEFAULT 0

This structural migration:

- does not insert the canonical row;
- does not change an existing active_phase;
- does not perform progression validation;
- does not repair an existing progression state.

For an already-current schema, the update leaves the existing
state unchanged.

The provisioning SQL is designed to be idempotent.

## Ownership rule

The table may be shared historically by V1 and GPA, but runtime
write authority must never be shared.

During normal operation exactly one progression authority may
write the canonical row:

    V1 active  -> GPA must not write
    GPA active -> V1 must not write

A production cutover therefore requires V1 progression authority
to be disabled before GPA becomes active.

## GPA load semantics

GPA deliberately does not inherit V1 silent-repair behavior.

Missing row:

    Validate configured default
    Save complete default state
    Initialize GPA

Valid persisted row:

    Load the exact persisted state
    Initialize GPA

Invalid persisted row:

    Fail progression initialization
    Do not repair the row
    Do not substitute an unrelated default
    Do not publish an in-memory authority

Storage/query error:

    Fail progression initialization
    Do not create an in-memory authority

The WorldDatabase adapter uses an aggregate query so that a
successful query returning no canonical row can be distinguished
from a query failure through AzerothCore's QueryResult API.

## GPA save semantics

GPA writes Phase and ContentStage as one complete state.

A normal GPA transition must never persist only one progression
axis.

The WorldDatabase implementation uses one idempotent UPSERT:

    INSERT INTO phase_progression_state
        (id, active_phase, active_content_stage)
    VALUES
        (1, Phase, ContentStage)
    ON DUPLICATE KEY UPDATE
        active_phase = Phase,
        active_content_stage = ContentStage

The adapter uses the generic Core API:

    DatabaseWorkerPool::DirectExecuteWithResult(...)

The current development Core implementation of that API is
checkpointed at:

    e404994515ec8844cc0e8f42bfa49f3f7e11fc6d

This is an external Core dependency until an upstream equivalent
exists.

## Save result semantics

Save returning true means the persistence backend reported that
the synchronous operation completed successfully.

Save returning false means persistence was not confirmed
successful.

A false result does not establish that the durable database state
is unchanged in every possible transport or connection-failure
scenario.

Therefore on Save failure GPA must:

    not publish the target state in memory
    report transition failure
    treat the durable outcome conservatively as unconfirmed

The complete UPSERT is idempotent. AzerothCore's database layer
may reconnect and retry applicable connection failures, so
repeating the same target converges on the same complete state.

After a restart, the persisted canonical row is loaded again and
becomes the source used for GPA initialization.

## Migration from V1

No data-copy migration is required from certified V1.

At cutover GPA consumes the same canonical row that V1 previously
owned.

The certified production state observed during development was:

    Phase = 20
    ContentStage = 0

That value is only a design-time observation.

The live row must always be revalidated during deployment and
documentation must never be treated as the current authority.

## Rollback

Because V1 and GPA use the same canonical persisted row, an
emergency rollback does not require synchronization between two
separate progression tables.

Rollback still requires exclusive runtime authority:

    stop GPA authority
    validate persisted state
    restore V1 authority
    start V1 using the same canonical row
