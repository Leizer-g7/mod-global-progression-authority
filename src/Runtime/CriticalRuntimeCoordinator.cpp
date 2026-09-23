#include "CriticalRuntimeCoordinator.h"

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
    bool CriticalRuntimeCoordinator::RegisterParticipant(
        ICriticalRuntimeParticipant& participant,
        std::string& error)
    {
        if (_state !=
            CriticalRuntimeCoordinatorState::Idle)
        {
            error =
                "Cannot register a critical runtime participant "
                "while a transaction is active.";

            return false;
        }

        std::string_view const name =
            participant.Name();

        if (name.empty())
        {
            error =
                "Critical runtime participant name cannot be empty.";

            return false;
        }

        for (ICriticalRuntimeParticipant const* existing :
             _participants)
        {
            if (existing == &participant ||
                existing->Name() == name)
            {
                error =
                    "Critical runtime participant is already registered: " +
                    std::string(name);

                return false;
            }
        }

        _participants.push_back(
            &participant);

        error.clear();

        return true;
    }

    bool CriticalRuntimeCoordinator::Prepare(
        ProgressionState const& target,
        std::string& error)
    {
        if (_state !=
            CriticalRuntimeCoordinatorState::Idle)
        {
            error =
                "Cannot prepare critical runtime while another "
                "transaction is active.";

            return false;
        }

        for (std::size_t index = 0;
             index < _participants.size();
             ++index)
        {
            ICriticalRuntimeParticipant* participant =
                _participants[index];

            std::string participantError;

            if (!participant->Prepare(
                    target,
                    participantError))
            {
                /*
                 * Prepare is contractually side-effect-free for runtime,
                 * but participants may already have captured snapshots.
                 * Discard every preparation attempted so far, including
                 * the participant that reported failure.
                 */
                for (std::size_t cancelIndex = 0;
                     cancelIndex <= index;
                     ++cancelIndex)
                {
                    _participants[cancelIndex]->
                        CancelPrepared();
                }

                error =
                    "Critical runtime prepare failed for " +
                    std::string(participant->Name()) + ".";

                AppendError(
                    error,
                    participantError);

                _state =
                    CriticalRuntimeCoordinatorState::Idle;

                return false;
            }
        }

        _state =
            CriticalRuntimeCoordinatorState::Prepared;

        error.clear();

        return true;
    }

    CriticalRuntimeApplyResult
    CriticalRuntimeCoordinator::ApplyPrepared()
    {
        if (_state !=
            CriticalRuntimeCoordinatorState::Prepared)
        {
            return {
                CriticalRuntimeApplyStatus::NotPrepared,
                {},
                "Critical runtime transaction is not prepared."
            };
        }

        for (std::size_t index = 0;
             index < _participants.size();
             ++index)
        {
            ICriticalRuntimeParticipant* participant =
                _participants[index];

            std::string applyError;

            if (!participant->ApplyPrepared(
                    applyError))
            {
                /*
                 * Include the participant that failed ApplyPrepared().
                 * Its contract explicitly permits partial mutation before
                 * returning false.
                 */
                std::string restoreParticipant;
                std::string restoreError;

                bool const restored =
                    RestoreRange(
                        index + 1,
                        restoreParticipant,
                        restoreError);

                std::string combinedError =
                    "Critical runtime apply failed for " +
                    std::string(participant->Name()) + ".";

                AppendError(
                    combinedError,
                    applyError);

                if (restored)
                {
                    CancelAllPrepared();

                    _state =
                        CriticalRuntimeCoordinatorState::Idle;

                    return {
                        CriticalRuntimeApplyStatus::
                            ApplyFailedRestored,
                        std::string(participant->Name()),
                        std::move(combinedError)
                    };
                }

                AppendError(
                    combinedError,
                    "Runtime restoration also failed.");

                AppendError(
                    combinedError,
                    restoreError);

                _state =
                    CriticalRuntimeCoordinatorState::
                        RecoveryFailed;

                return {
                    CriticalRuntimeApplyStatus::
                        ApplyFailedRestoreFailed,
                    restoreParticipant.empty()
                        ? std::string(participant->Name())
                        : std::move(restoreParticipant),
                    std::move(combinedError)
                };
            }
        }

        _state =
            CriticalRuntimeCoordinatorState::Applied;

        return {
            CriticalRuntimeApplyStatus::Applied,
            {},
            {}
        };
    }

    CriticalRuntimeRestoreResult
    CriticalRuntimeCoordinator::RestorePrepared()
    {
        if (_state !=
                CriticalRuntimeCoordinatorState::Applied &&
            _state !=
                CriticalRuntimeCoordinatorState::RecoveryFailed)
        {
            return {
                CriticalRuntimeRestoreStatus::NotApplied,
                {},
                "Critical runtime transaction is not applied."
            };
        }

        std::string failedParticipant;
        std::string restoreError;

        if (!RestoreRange(
                _participants.size(),
                failedParticipant,
                restoreError))
        {
            _state =
                CriticalRuntimeCoordinatorState::
                    RecoveryFailed;

            return {
                CriticalRuntimeRestoreStatus::RestoreFailed,
                std::move(failedParticipant),
                std::move(restoreError)
            };
        }

        CancelAllPrepared();

        _state =
            CriticalRuntimeCoordinatorState::Idle;

        return {
            CriticalRuntimeRestoreStatus::Restored,
            {},
            {}
        };
    }

    bool CriticalRuntimeCoordinator::CommitPrepared(
        std::string& error)
    {
        if (_state !=
            CriticalRuntimeCoordinatorState::Applied)
        {
            error =
                "Cannot commit critical runtime because no applied "
                "transaction is pending.";

            return false;
        }

        for (ICriticalRuntimeParticipant* participant :
             _participants)
        {
            participant->CommitPrepared();
        }

        _state =
            CriticalRuntimeCoordinatorState::Idle;

        error.clear();

        return true;
    }

    void CriticalRuntimeCoordinator::CancelPrepared()
    {
        if (_state !=
            CriticalRuntimeCoordinatorState::Prepared)
        {
            return;
        }

        CancelAllPrepared();

        _state =
            CriticalRuntimeCoordinatorState::Idle;
    }

    CriticalRuntimeCoordinatorState
    CriticalRuntimeCoordinator::GetState() const
    {
        return _state;
    }

    std::size_t
    CriticalRuntimeCoordinator::ParticipantCount() const
    {
        return _participants.size();
    }

    bool CriticalRuntimeCoordinator::RestoreRange(
        std::size_t count,
        std::string& firstFailedParticipant,
        std::string& error)
    {
        bool success = true;

        firstFailedParticipant.clear();
        error.clear();

        while (count > 0)
        {
            --count;

            ICriticalRuntimeParticipant* participant =
                _participants[count];

            std::string participantError;

            if (!participant->RestorePrepared(
                    participantError))
            {
                if (firstFailedParticipant.empty())
                {
                    firstFailedParticipant =
                        std::string(participant->Name());
                }

                success = false;

                std::string message =
                    "Restore failed for " +
                    std::string(participant->Name()) + ".";

                AppendError(
                    message,
                    participantError);

                AppendError(
                    error,
                    message);
            }
        }

        return success;
    }

    void CriticalRuntimeCoordinator::CancelAllPrepared()
    {
        for (ICriticalRuntimeParticipant* participant :
             _participants)
        {
            participant->CancelPrepared();
        }
    }
}
