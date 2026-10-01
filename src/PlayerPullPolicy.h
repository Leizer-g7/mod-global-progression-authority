#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PLAYER_PULL_POLICY_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PLAYER_PULL_POLICY_H

#include "PullPolicy.h"

#include <cstdint>

namespace GPA
{
    enum class PlayerPullPolicyMode : std::uint8_t
    {
        Bypass,
        EnforceKnownCap,
        BlockProgression
    };

    constexpr PlayerPullPolicyMode ClassifyPlayerPullPolicy(
        PullPolicyStatus status,
        bool hasDefinition,
        bool gmBypass,
        bool isGameMaster) noexcept
    {
        if (status == PullPolicyStatus::Disabled)
            return PlayerPullPolicyMode::Bypass;

        if (gmBypass && isGameMaster)
            return PlayerPullPolicyMode::Bypass;

        if ((status == PullPolicyStatus::Ready ||
             status == PullPolicyStatus::ActiveFailClosed) &&
            hasDefinition)
        {
            return PlayerPullPolicyMode::EnforceKnownCap;
        }

        return PlayerPullPolicyMode::BlockProgression;
    }

    inline PlayerPullPolicyMode ClassifyPlayerPullPolicy(
        PullPolicySnapshot const& snapshot,
        bool isGameMaster) noexcept
    {
        return ClassifyPlayerPullPolicy(
            snapshot.status,
            snapshot.definition.has_value(),
            snapshot.gmBypass,
            isGameMaster);
    }
}

#endif
