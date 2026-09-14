#include "MemoryProgressionStateStore.h"
#include "ProgressionTransitionCoordinator.h"

#include <iostream>
#include <string>

namespace
{
    bool Init(
        GPA::ProgressionMgr& manager,
        GPA::ProgressionState const& state,
        bool allowRollback)
    {
        std::string error;

        return manager.Initialize(
            state,
            allowRollback,
            error);
    }

    bool StateEquals(
        GPA::ProgressionMgr const& manager,
        std::uint8_t phase,
        std::uint8_t stage)
    {
        auto state =
            manager.GetActiveState();

        return state &&
            state->phase == phase &&
            state->contentStage == stage;
    }
}

int main()
{
    using GPA::PersistentTransitionStatus;
    using GPA::ProgressionMgr;
    using GPA::ProgressionTransitionCoordinator;
    using GPA::Test::MemoryProgressionStateStore;

    /*
     * Manager not initialized.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({60, 0});

        ProgressionTransitionCoordinator coordinator(
            manager,
            store);

        auto result =
            coordinator.Execute({60, 1});

        if (result.status !=
                PersistentTransitionStatus::NotInitialized ||
            result.Succeeded() ||
            manager.IsInitialized() ||
            store.GetSaveCount() != 0)
        {
            return 1;
        }
    }

    /*
     * Invalid target never reaches persistence.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({60, 0});

        if (!Init(manager, {60, 0}, false))
            return 2;

        ProgressionTransitionCoordinator coordinator(
            manager,
            store);

        auto result =
            coordinator.Execute({60, 9});

        if (result.status !=
                PersistentTransitionStatus::InvalidTarget ||
            result.Succeeded() ||
            !StateEquals(manager, 60, 0) ||
            store.GetSaveCount() != 0)
        {
            return 3;
        }
    }

    /*
     * NoChange is successful but performs no Save.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({60, 0});

        if (!Init(manager, {60, 0}, false))
            return 4;

        ProgressionTransitionCoordinator coordinator(
            manager,
            store);

        auto result =
            coordinator.Execute({60, 0});

        if (result.status !=
                PersistentTransitionStatus::NoChange ||
            !result.Succeeded() ||
            result.Changed() ||
            store.GetSaveCount() != 0)
        {
            return 5;
        }
    }

    /*
     * Save failure leaves active authority unchanged.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({60, 0});

        if (!Init(manager, {60, 0}, false))
            return 6;

        store.SetSaveError(
            "simulated transition save failure");

        ProgressionTransitionCoordinator coordinator(
            manager,
            store);

        auto result =
            coordinator.Execute({60, 1});

        if (result.status !=
                PersistentTransitionStatus::SaveFailed ||
            result.error.empty() ||
            result.Succeeded() ||
            !StateEquals(manager, 60, 0) ||
            store.GetSaveCount() != 0)
        {
            return 7;
        }
    }

    /*
     * Successful transition persists before publication.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({60, 0});

        if (!Init(manager, {60, 0}, false))
            return 8;

        ProgressionTransitionCoordinator coordinator(
            manager,
            store);

        auto result =
            coordinator.Execute({60, 1});

        if (result.status !=
                PersistentTransitionStatus::Applied ||
            !result.Succeeded() ||
            !result.Changed() ||
            !StateEquals(manager, 60, 1) ||
            store.GetSaveCount() != 1 ||
            !store.GetStoredState() ||
            *store.GetStoredState() !=
                GPA::ProgressionState{60, 1})
        {
            return 9;
        }
    }

    /*
     * Rollback blocked before persistence.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({60, 1});

        if (!Init(manager, {60, 1}, false))
            return 10;

        ProgressionTransitionCoordinator coordinator(
            manager,
            store);

        auto result =
            coordinator.Execute({60, 0});

        if (result.status !=
                PersistentTransitionStatus::RollbackNotAllowed ||
            !StateEquals(manager, 60, 1) ||
            store.GetSaveCount() != 0)
        {
            return 11;
        }
    }

    /*
     * Explicitly allowed rollback is persisted and published.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({70, 13});

        if (!Init(manager, {70, 13}, true))
            return 12;

        ProgressionTransitionCoordinator coordinator(
            manager,
            store);

        auto result =
            coordinator.Execute({60, 8});

        if (result.status !=
                PersistentTransitionStatus::Applied ||
            !result.Succeeded() ||
            !StateEquals(manager, 60, 8) ||
            store.GetSaveCount() != 1 ||
            !store.GetStoredState() ||
            *store.GetStoredState() !=
                GPA::ProgressionState{60, 8})
        {
            return 13;
        }
    }

    std::cout
        << "COORDINATOR_NOT_INITIALIZED=OK\n"
        << "COORDINATOR_INVALID_TARGET=OK\n"
        << "COORDINATOR_NOCHANGE_NO_SAVE=OK\n"
        << "COORDINATOR_SAVE_FAILURE_ATOMICITY=OK\n"
        << "COORDINATOR_APPLIED=OK\n"
        << "COORDINATOR_ROLLBACK_BLOCKED=OK\n"
        << "COORDINATOR_ROLLBACK_ALLOWED=OK\n";

    return 0;
}
