#include "ProgressionMgr.h"

namespace
{
    GPA::TransitionDirection GetDirection(
        GPA::ProgressionState const& current,
        GPA::ProgressionState const& target)
    {
        if (current == target)
            return GPA::TransitionDirection::NoChange;

        /*
         * GPA has two progression axes:
         *
         *   Phase
         *   ContentStage
         *
         * If either axis moves backwards, the whole transition
         * is treated as rollback.
         */
        if (target.phase < current.phase ||
            target.contentStage < current.contentStage)
        {
            return GPA::TransitionDirection::Rollback;
        }

        return GPA::TransitionDirection::Forward;
    }
}

namespace GPA
{
    ProgressionMgr::ProgressionMgr() = default;

    bool ProgressionMgr::Initialize(
        ProgressionState const& initialState,
        bool allowRollback,
        std::string& error)
    {
        if (_activeState)
        {
            error =
                "ProgressionMgr is already initialized.";

            return false;
        }

        if (!ValidateState(initialState, error))
            return false;

        _activeState =
            initialState;

        _allowRollback =
            allowRollback;

        error.clear();

        return true;
    }

    bool ProgressionMgr::IsInitialized() const
    {
        return _activeState.has_value();
    }

    bool ProgressionMgr::IsRollbackAllowed() const
    {
        return _allowRollback;
    }

    bool ProgressionMgr::ValidateState(
        ProgressionState const& state,
        std::string& error) const
    {
        if (!_catalog.Validate(error))
            return false;

        if (!_catalog.IsValidState(state))
        {
            error =
                "Progression state is invalid.";

            return false;
        }

        error.clear();
        return true;
    }

    ProgressionState const*
    ProgressionMgr::GetActiveState() const
    {
        if (!_activeState)
            return nullptr;

        return &*_activeState;
    }

    PhaseDefinition const*
    ProgressionMgr::GetActiveDefinition() const
    {
        if (!_activeState)
            return nullptr;

        return _catalog.Find(
            _activeState->phase);
    }

    TransitionResult
    ProgressionMgr::EvaluateTransition(
        ProgressionState const& target) const
    {
        TransitionResult result;

        result.target =
            target;

        if (!_activeState)
        {
            result.status =
                TransitionStatus::NotInitialized;

            return result;
        }

        result.previous =
            *_activeState;

        if (!_catalog.IsValidState(target))
        {
            result.status =
                TransitionStatus::InvalidTarget;

            return result;
        }

        result.direction =
            GetDirection(
                *_activeState,
                target);

        if (result.direction ==
            TransitionDirection::NoChange)
        {
            result.status =
                TransitionStatus::NoChange;

            return result;
        }

        if (result.direction ==
                TransitionDirection::Rollback &&
            !_allowRollback)
        {
            result.status =
                TransitionStatus::RollbackNotAllowed;

            return result;
        }

        result.status =
            TransitionStatus::Allowed;

        return result;
    }

    TransitionResult
    ProgressionMgr::RequestTransition(
        ProgressionState const& target)
    {
        TransitionResult result =
            EvaluateTransition(target);

        if (result.status !=
            TransitionStatus::Allowed)
        {
            return result;
        }

        _activeState =
            target;

        result.status =
            TransitionStatus::Applied;

        return result;
    }
}
