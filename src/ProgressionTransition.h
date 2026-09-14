#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_TRANSITION_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_TRANSITION_H

#include "ProgressionState.h"

#include <optional>

namespace GPA
{
    enum class TransitionDirection
    {
        NoChange,
        Forward,
        Rollback
    };

    enum class TransitionStatus
    {
        Allowed,
        Applied,
        NoChange,
        NotInitialized,
        InvalidTarget,
        RollbackNotAllowed
    };

    struct TransitionResult
    {
        TransitionStatus status =
            TransitionStatus::NotInitialized;

        TransitionDirection direction =
            TransitionDirection::NoChange;

        std::optional<ProgressionState> previous;

        ProgressionState target;

        bool Succeeded() const
        {
            return
                status == TransitionStatus::Allowed ||
                status == TransitionStatus::Applied ||
                status == TransitionStatus::NoChange;
        }

        bool Changed() const
        {
            return
                status == TransitionStatus::Applied;
        }
    };
}

#endif
