#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_TRANSITION_COORDINATOR_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_TRANSITION_COORDINATOR_H

#include "ProgressionMgr.h"
#include "ProgressionStateStore.h"
#include "Runtime/CriticalRuntimeCoordinator.h"

#include <string>

namespace GPA
{
    enum class PersistentTransitionStatus
    {
        Applied,
        NoChange,

        NotInitialized,
        AuthorityUnavailable,
        InvalidTarget,
        RollbackNotAllowed,

        RuntimePrepareFailed,

        /*
         * Target runtime application failed, but the previous
         * runtime snapshot was restored successfully.
         */
        RuntimeApplyFailed,

        /*
         * Target runtime application failed and restoration of
         * the previous runtime also failed.
         */
        RuntimeRecoveryFailed,

        /*
         * Persistence failed after runtime application, but the
         * previous runtime snapshot was restored successfully.
         */
        SaveFailed,

        /*
         * Persistence failed and restoration of the previous
         * runtime also failed.
         */
        SaveFailedRuntimeRecoveryFailed,

        /*
         * Persistence, GPA memory publication and runtime target
         * application succeeded, but the runtime transaction could
         * not be finalized. Authority must remain fail-closed.
         */
        RuntimeFinalizeFailed
    };

    struct PersistentTransitionResult
    {
        PersistentTransitionStatus status =
            PersistentTransitionStatus::NotInitialized;

        TransitionResult transition;

        std::string error;

        bool Succeeded() const
        {
            return
                status == PersistentTransitionStatus::Applied ||
                status == PersistentTransitionStatus::NoChange;
        }

        bool Changed() const
        {
            return
                status == PersistentTransitionStatus::Applied;
        }

        /*
         * Whether GPA memory and critical runtime are known to be
         * mutually consistent after this result.
         *
         * This does not claim anything stronger about persistence
         * after a Save() failure, whose contract explicitly permits
         * an uncertain durable outcome.
         */
        bool RuntimeConsistent() const
        {
            switch (status)
            {
                case PersistentTransitionStatus::Applied:
                case PersistentTransitionStatus::NoChange:
                case PersistentTransitionStatus::InvalidTarget:
                case PersistentTransitionStatus::RollbackNotAllowed:
                case PersistentTransitionStatus::RuntimePrepareFailed:
                case PersistentTransitionStatus::RuntimeApplyFailed:
                case PersistentTransitionStatus::SaveFailed:
                    return true;

                case PersistentTransitionStatus::NotInitialized:
                case PersistentTransitionStatus::AuthorityUnavailable:
                case PersistentTransitionStatus::RuntimeRecoveryFailed:
                case PersistentTransitionStatus::
                    SaveFailedRuntimeRecoveryFailed:
                case PersistentTransitionStatus::RuntimeFinalizeFailed:
                    return false;
            }

            return false;
        }

        /*
         * Whether the global authority may safely be published again
         * after this result.
         *
         * This is deliberately stronger than RuntimeConsistent().
         * SaveFailed may leave GPA memory and critical runtime mutually
         * coherent while the durable backend outcome remains uncertain.
         */
        bool SafeToRepublishAuthority() const
        {
            switch (status)
            {
                case PersistentTransitionStatus::Applied:
                case PersistentTransitionStatus::NoChange:
                case PersistentTransitionStatus::InvalidTarget:
                case PersistentTransitionStatus::RollbackNotAllowed:
                case PersistentTransitionStatus::RuntimePrepareFailed:
                case PersistentTransitionStatus::RuntimeApplyFailed:
                    return true;

                case PersistentTransitionStatus::NotInitialized:
                case PersistentTransitionStatus::AuthorityUnavailable:
                case PersistentTransitionStatus::RuntimeRecoveryFailed:
                case PersistentTransitionStatus::SaveFailed:
                case PersistentTransitionStatus::
                    SaveFailedRuntimeRecoveryFailed:
                case PersistentTransitionStatus::RuntimeFinalizeFailed:
                    return false;
            }

            return false;
        }
    };

    class ProgressionTransitionCoordinator
    {
    public:
        ProgressionTransitionCoordinator(
            ProgressionMgr& manager,
            IProgressionStateStore& store,
            CriticalRuntimeCoordinator& criticalRuntime);

        PersistentTransitionResult Execute(
            ProgressionState const& target);

    private:
        ProgressionMgr& _manager;
        IProgressionStateStore& _store;
        CriticalRuntimeCoordinator& _criticalRuntime;
    };
}

#endif
