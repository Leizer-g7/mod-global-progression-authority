# GPA Persistent Transitions

## Authority rule

A normal GPA progression change is a complete state transition:

    { Phase, ContentStage } -> { Phase, ContentStage }

The durable store and the in-memory authority must not be changed
independently by normal callers.

The final public write path will be coordinated by GPA.

## Required sequence

For an initialized GPA manager:

1. Evaluate the complete target state.
2. Reject invalid or forbidden transitions without persistence.
3. Treat NoChange as success without persistence.
4. Persist the complete target state.
5. Only after successful persistence, publish the target in memory.

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
        +-- failure --> active state remains unchanged
        |
        +-- success --> commit the same target in memory

## Persistence failure

If Save(target) fails:

- the active in-memory state must remain unchanged;
- the transition must report failure;
- no silent fallback or repair is allowed.

## Serialization

GPA transitions must be serialized.

No second progression write may occur between evaluation,
persistence, and in-memory publication of one transition.

This prevents another writer from invalidating the evaluated
previous/target pair.

## Post-save publication invariant

The coordinator holds the manager write lock across evaluation,
persistence, and in-memory publication.

After Save(target) succeeds, the exact already-evaluated target
is committed directly in memory without a second validation pass.

No other GPA writer can modify the manager between those steps.

Therefore runtime publication is deterministic in the current
design and does not require a second persistence write or a
best-effort restoration path.

## Restart semantics

The persistent state is the startup source of authority.

If the process stops after Save(target) but before runtime
publication completes, the next startup loads and validates
the persisted target.

Startup adapters will later be responsible for applying the
loaded authoritative state to their runtime subsystems.

## Future adapter transaction

Persistent transition handling is only the persistence layer.

When adapters are introduced, the full transaction will extend
this model with adapter preflight, apply, and critical-failure
recovery. Persistence policy must remain centralized in GPA.
