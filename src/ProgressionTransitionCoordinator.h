#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_TRANSITION_COORDINATOR_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_TRANSITION_COORDINATOR_H

#include "ProgressionMgr.h"
#include "ProgressionStateStore.h"

#include <string>

namespace GPA
{
    enum class PersistentTransitionStatus
    {
        Applied,
        NoChange,

        NotInitialized,
        InvalidTarget,
        RollbackNotAllowed,

        SaveFailed
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

    };

    class ProgressionTransitionCoordinator
    {
    public:
        ProgressionTransitionCoordinator(
            ProgressionMgr& manager,
            IProgressionStateStore& store);

        PersistentTransitionResult Execute(
            ProgressionState const& target);

    private:
        ProgressionMgr& _manager;
        IProgressionStateStore& _store;
    };
}

#endif
