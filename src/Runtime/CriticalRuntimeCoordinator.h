#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_CRITICAL_RUNTIME_COORDINATOR_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_CRITICAL_RUNTIME_COORDINATOR_H

#include "ProgressionState.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace GPA
{
    /*
     * One critical runtime participant.
     *
     * Prepare():
     *   - must not mutate runtime state;
     *   - validates the target;
     *   - captures enough current runtime state to restore it exactly.
     *
     * ApplyPrepared():
     *   - applies the target prepared above;
     *   - on failure it may have partially mutated runtime state;
     *   - therefore RestorePrepared() must be safe even after a failed
     *     ApplyPrepared().
     *
     * RestorePrepared():
     *   - restores the snapshot captured by Prepare();
     *   - should be retry-safe until CommitPrepared()/CancelPrepared().
     *
     * CommitPrepared():
     *   - discards the captured snapshot after persistence/memory commit.
     *
     * CancelPrepared():
     *   - discards preparation that never became authoritative.
     */
    class ICriticalRuntimeParticipant
    {
    public:
        virtual ~ICriticalRuntimeParticipant() = default;

        virtual std::string_view Name() const = 0;

        virtual bool Prepare(
            ProgressionState const& target,
            std::string& error) = 0;

        virtual bool ApplyPrepared(
            std::string& error) = 0;

        virtual bool RestorePrepared(
            std::string& error) = 0;

        virtual void CommitPrepared() = 0;

        virtual void CancelPrepared() = 0;
    };

    enum class CriticalRuntimeCoordinatorState
    {
        Idle,
        Prepared,
        Applied,
        RecoveryFailed
    };

    enum class CriticalRuntimeApplyStatus
    {
        Applied,
        NotPrepared,
        ApplyFailedRestored,
        ApplyFailedRestoreFailed
    };

    struct CriticalRuntimeApplyResult
    {
        CriticalRuntimeApplyStatus status =
            CriticalRuntimeApplyStatus::NotPrepared;

        std::string participant;
        std::string error;

        bool Succeeded() const
        {
            return status ==
                CriticalRuntimeApplyStatus::Applied;
        }

        bool RuntimeRestored() const
        {
            return status ==
                CriticalRuntimeApplyStatus::ApplyFailedRestored;
        }
    };

    enum class CriticalRuntimeRestoreStatus
    {
        Restored,
        NotApplied,
        RestoreFailed
    };

    struct CriticalRuntimeRestoreResult
    {
        CriticalRuntimeRestoreStatus status =
            CriticalRuntimeRestoreStatus::NotApplied;

        std::string participant;
        std::string error;

        bool Succeeded() const
        {
            return status ==
                CriticalRuntimeRestoreStatus::Restored;
        }
    };

    class CriticalRuntimeCoordinator
    {
    public:
        bool RegisterParticipant(
            ICriticalRuntimeParticipant& participant,
            std::string& error);

        bool Prepare(
            ProgressionState const& target,
            std::string& error);

        CriticalRuntimeApplyResult ApplyPrepared();

        CriticalRuntimeRestoreResult RestorePrepared();

        bool CommitPrepared(
            std::string& error);

        void CancelPrepared();

        CriticalRuntimeCoordinatorState GetState() const;

        std::size_t ParticipantCount() const;

    private:
        bool RestoreRange(
            std::size_t count,
            std::string& firstFailedParticipant,
            std::string& error);

        void CancelAllPrepared();

        std::vector<ICriticalRuntimeParticipant*> _participants;

        /*
         * Number of participants for which ApplyPrepared() has
         * actually been attempted in the current transaction.
         *
         * On recovery retry, only this prefix may receive
         * RestorePrepared(). Participants after this prefix were
         * prepared but never applied.
         */
        std::size_t _restoreCount = 0;

        CriticalRuntimeCoordinatorState _state =
            CriticalRuntimeCoordinatorState::Idle;
    };
}

#endif
