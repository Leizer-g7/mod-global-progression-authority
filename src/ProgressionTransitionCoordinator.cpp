#include "ProgressionTransitionCoordinator.h"

#include <mutex>

namespace GPA
{
    ProgressionTransitionCoordinator::
    ProgressionTransitionCoordinator(
        ProgressionMgr& manager,
        IProgressionStateStore& store)
        : _manager(manager),
          _store(store)
    {
    }

    PersistentTransitionResult
    ProgressionTransitionCoordinator::Execute(
        ProgressionState const& target)
    {
        std::lock_guard<std::mutex> lock(
            _manager._transitionMutex);

        TransitionResult evaluation =
            _manager.EvaluateTransitionUnlocked(target);

        if (evaluation.status ==
            TransitionStatus::NotInitialized)
        {
            return {
                PersistentTransitionStatus::NotInitialized,
                evaluation,
                "ProgressionMgr is not initialized."
            };
        }

        if (evaluation.status ==
            TransitionStatus::InvalidTarget)
        {
            return {
                PersistentTransitionStatus::InvalidTarget,
                evaluation,
                "Target progression state is invalid."
            };
        }

        if (evaluation.status ==
            TransitionStatus::RollbackNotAllowed)
        {
            return {
                PersistentTransitionStatus::RollbackNotAllowed,
                evaluation,
                "Rollback is not allowed."
            };
        }

        if (evaluation.status ==
            TransitionStatus::NoChange)
        {
            return {
                PersistentTransitionStatus::NoChange,
                evaluation,
                {}
            };
        }

        /*
         * EvaluateTransition() must return Allowed here.
         * Anything else would violate the ProgressionMgr contract.
         */
        if (evaluation.status !=
            TransitionStatus::Allowed)
        {
            return {
                PersistentTransitionStatus::InvalidTarget,
                evaluation,
                "Unexpected transition evaluation status."
            };
        }

        /*
         * Allowed transitions always have a previous state.
         * Verify that invariant before changing persistence.
         */
        if (!evaluation.previous)
        {
            return {
                PersistentTransitionStatus::NotInitialized,
                evaluation,
                "Allowed transition has no previous state."
            };
        }

        std::string saveError;

        if (!_store.Save(
                target,
                saveError))
        {
            if (saveError.empty())
                saveError =
                    "State store failed to persist target state.";

            return {
                PersistentTransitionStatus::SaveFailed,
                evaluation,
                saveError
            };
        }

        /*
         * Persistence succeeded.
         *
         * Publish exactly the same target in memory.
         */
        _manager.CommitEvaluatedTransitionUnlocked(
            evaluation);

        evaluation.status =
            TransitionStatus::Applied;

        return {
            PersistentTransitionStatus::Applied,
            evaluation,
            {}
        };
    }
}
