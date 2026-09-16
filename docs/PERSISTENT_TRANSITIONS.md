# GPA Persistent Transitions

## Authority rule

A normal GPA progression change is a complete state transition:

    { Phase, ContentStage } -> { Phase, ContentStage }

The durable store and the in-memory authority must not be changed
independently by normal callers.

The public write path is coordinated by GPA.

## Required sequence

For an initialized GPA manager:

1. Evaluate the complete target state.
2. Reject invalid or forbidden transitions without persistence.
3. Treat NoChange as success without persistence.
4. Persist the complete target state.
5. Only after confirmed persistence success, publish the target
   in memory.

Therefore:

    Evaluate
        |
        +-- rejected --> no Save, no memory change
        |
        +-- NoChange --> no Save, no memory change
        |
        +-- Allowed
               |
               v
             Save
               |
        +------+
        |
        +-- false --> no target publication
        |
        +-- true  --> commit exact target in memory

## Persistence failure

If Save(target) returns false:

- persistence was not confirmed successful;
- the active in-memory state remains unchanged;
- the target is not published in memory;
- the transition reports failure;
- no silent fallback or repair is allowed.

Save returning false must not be interpreted as universal proof
that the durable database state remained unchanged.

The persisted state is therefore re-established as authoritative
through the normal startup load path after a restart.

## Serialization

GPA bootstrap and transitions are serialized through the same
ProgressionMgr authority mutex.

No second GPA progression write may occur between evaluation,
persistence, and in-memory publication of one transition.

This prevents another GPA writer from invalidating the evaluated
previous/target pair.

## Persistence-store reentrancy

ProgressionBootstrap and ProgressionTransitionCoordinator may call
IProgressionStateStore while holding the ProgressionMgr authority
mutex.

A persistence-store implementation therefore must not call back
into ProgressionMgr, ProgressionBootstrap,
ProgressionTransitionCoordinator, or another path that attempts
to acquire the same authority mutex.

The WorldDatabase state store is intentionally independent from
those classes and performs only persistence operations.

## Post-save publication invariant

The coordinator holds the manager authority lock across:

    evaluation
    persistence
    memory publication

After Save(target) returns true, the exact already-evaluated
target is committed directly in memory without another validation
pass.

No other GPA writer can modify the manager between those steps.

There is no best-effort post-save restoration write.

## Restart semantics

The persisted canonical state is the startup source used for GPA
initialization.

If the process stops after Save(target) has completed but before
runtime publication completes, the next startup loads and
validates the persisted target.

Startup adapters will later be responsible for applying the
loaded authoritative state to their runtime subsystems.

## Future adapter transaction

Persistent transition handling covers the persistence layer.

When runtime adapters are introduced, the full transaction will
extend this model with adapter preflight, application, and
critical-failure handling.

Persistence policy and global write authority remain centralized
in GPA.
