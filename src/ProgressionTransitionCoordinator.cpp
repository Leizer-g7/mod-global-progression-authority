#include "ProgressionTransitionCoordinator.h"

#include <mutex>
#include <utility>

namespace
{
    void AppendError(
        std::string& target,
        std::string const& text)
    {
        if (text.empty())
            return;

        if (!target.empty())
            target += " ";

        target += text;
    }
}

namespace GPA
{
    ProgressionTransitionCoordinator::
    ProgressionTransitionCoordinator(
        ProgressionMgr& manager,
        IProgressionStateStore& store,
        CriticalRuntimeCoordinator& criticalRuntime)
        : _manager(manager),
          _store(store),
          _criticalRuntime(criticalRuntime)
    {
    }

    PersistentTransitionResult
    ProgressionTransitionCoordinator::Execute(
        ProgressionState const& target)
    {
        /*
         * One authority lock covers:
         *
         *   Evaluate
         *   -> Prepare runtime
         *   -> Apply runtime
         *   -> Save persistence
         *   -> Commit GPA memory
         *   -> Finalize runtime transaction
         *
         * Critical runtime participants must therefore never
         * re-enter ProgressionMgr or this coordinator while one
         * of their transaction callbacks is executing.
         */
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
         */
        if (!evaluation.previous)
        {
            return {
                PersistentTransitionStatus::NotInitialized,
                evaluation,
                "Allowed transition has no previous state."
            };
        }

        /*
         * Prepare validates the target for every critical runtime
         * participant and captures each participant's exact current
         * runtime snapshot. Prepare itself must not mutate runtime.
         */
        std::string runtimeError;

        if (!_criticalRuntime.Prepare(
                target,
                runtimeError))
        {
            if (runtimeError.empty())
            {
                runtimeError =
                    "Critical runtime preparation failed.";
            }

            return {
                PersistentTransitionStatus::RuntimePrepareFailed,
                evaluation,
                runtimeError
            };
        }

        /*
         * Runtime changes happen before persistence.
         *
         * ApplyPrepared() automatically attempts reverse-order
         * restoration if any participant fails while applying.
         */
        CriticalRuntimeApplyResult const applyResult =
            _criticalRuntime.ApplyPrepared();

        if (!applyResult.Succeeded())
        {
            std::string error =
                applyResult.error.empty()
                    ? "Critical runtime application failed."
                    : applyResult.error;

            if (applyResult.status ==
                CriticalRuntimeApplyStatus::
                    ApplyFailedRestored)
            {
                return {
                    PersistentTransitionStatus::
                        RuntimeApplyFailed,
                    evaluation,
                    std::move(error)
                };
            }

            /*
             * NotPrepared after a successful Prepare() would also
             * violate the lifecycle contract, so fail closed.
             */
            return {
                PersistentTransitionStatus::
                    RuntimeRecoveryFailed,
                evaluation,
                std::move(error)
            };
        }

        std::string saveError;

        if (!_store.Save(
                target,
                saveError))
        {
            if (saveError.empty())
            {
                saveError =
                    "State store failed to persist target state.";
            }

            /*
             * GPA memory still contains the previous state because
             * publication has not happened yet.
             *
             * Restore the exact runtime snapshot captured during
             * Prepare(), not merely a reconstruction from the old
             * GPA phase.
             */
            CriticalRuntimeRestoreResult const restoreResult =
                _criticalRuntime.RestorePrepared();

            if (restoreResult.Succeeded())
            {
                return {
                    PersistentTransitionStatus::SaveFailed,
                    evaluation,
                    saveError
                };
            }

            std::string combinedError =
                saveError;

            AppendError(
                combinedError,
                "Critical runtime restoration after persistence "
                "failure also failed.");

            AppendError(
                combinedError,
                restoreResult.error);

            return {
                PersistentTransitionStatus::
                    SaveFailedRuntimeRecoveryFailed,
                evaluation,
                std::move(combinedError)
            };
        }

        /*
         * Persistence succeeded.
         *
         * Publish exactly the same evaluated target in GPA memory.
         * CommitEvaluatedTransitionUnlocked() is intentionally
         * non-failing.
         */
        _manager.CommitEvaluatedTransitionUnlocked(
            evaluation);

        evaluation.status =
            TransitionStatus::Applied;

        /*
         * The target is now:
         *
         *   - applied to critical runtime,
         *   - persisted,
         *   - published in GPA memory.
         *
         * Discard the rollback snapshots only now.
         */
        std::string finalizeError;

        if (!_criticalRuntime.CommitPrepared(
                finalizeError))
        {
            if (finalizeError.empty())
            {
                finalizeError =
                    "Critical runtime transaction finalization failed.";
            }

            return {
                PersistentTransitionStatus::
                    RuntimeFinalizeFailed,
                evaluation,
                finalizeError
            };
        }

        return {
            PersistentTransitionStatus::Applied,
            evaluation,
            {}
        };
    }
}
