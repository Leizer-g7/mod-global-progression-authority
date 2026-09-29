#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PULL_POLICY_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PULL_POLICY_H

#include "PhaseDefinition.h"

#include <cstdint>
#include <optional>

namespace GPA
{
    enum class PullPolicyStatus : std::uint8_t
    {
        Disabled,
        Ready,
        ActiveFailClosed,
        Unavailable
    };

    struct PullPolicySnapshot
    {
        PullPolicyStatus status =
            PullPolicyStatus::Unavailable;

        bool gmBypass = true;

        std::optional<PhaseDefinition> definition;

        bool HasActivePolicy() const noexcept
        {
            return
                status == PullPolicyStatus::Ready ||
                status == PullPolicyStatus::ActiveFailClosed;
        }

        bool IsFailClosed() const noexcept
        {
            return
                status == PullPolicyStatus::ActiveFailClosed ||
                status == PullPolicyStatus::Unavailable;
        }
    };

    constexpr PullPolicyStatus ClassifyPullPolicy(
        bool configLoaded,
        bool enabled,
        bool configValid,
        bool hasActiveDefinition,
        bool authorityReady) noexcept
    {
        if (!configLoaded)
            return PullPolicyStatus::Unavailable;

        if (!enabled)
            return PullPolicyStatus::Disabled;

        if (!configValid || !hasActiveDefinition)
            return PullPolicyStatus::Unavailable;

        return authorityReady
            ? PullPolicyStatus::Ready
            : PullPolicyStatus::ActiveFailClosed;
    }

    PullPolicySnapshot GetPullPolicySnapshot();
}

#endif
