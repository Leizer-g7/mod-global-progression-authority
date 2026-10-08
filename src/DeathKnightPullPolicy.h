#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_DEATH_KNIGHT_PULL_POLICY_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_DEATH_KNIGHT_PULL_POLICY_H

#include "PullPolicy.h"

#include <cstdint>

namespace GPA
{
    enum class DeathKnightCreationDecision : std::uint8_t
    {
        Allow,
        Block
    };

    constexpr DeathKnightCreationDecision
    ClassifyDeathKnightCreationPolicy(
        PullPolicyStatus status,
        bool hasDefinition,
        bool deathKnightEnabled,
        bool isDeathKnight) noexcept
    {
        /*
         * The progression authority does not manage ordinary
         * character classes here.
         */
        if (!isDeathKnight)
            return DeathKnightCreationDecision::Allow;

        /*
         * V1 parity:
         * when progression itself is disabled, DK creation
         * falls back to normal AzerothCore behaviour.
         */
        if (status == PullPolicyStatus::Disabled)
            return DeathKnightCreationDecision::Allow;

        /*
         * Ready and ActiveFailClosed both represent an active
         * authoritative policy when a definition is known.
         */
        if ((status == PullPolicyStatus::Ready ||
             status == PullPolicyStatus::ActiveFailClosed) &&
            hasDefinition)
        {
            return deathKnightEnabled
                ? DeathKnightCreationDecision::Allow
                : DeathKnightCreationDecision::Block;
        }

        /*
         * Unknown authority state must never silently unlock
         * an expansion class.
         */
        return DeathKnightCreationDecision::Block;
    }

    inline DeathKnightCreationDecision
    ClassifyDeathKnightCreationPolicy(
        PullPolicySnapshot const& snapshot,
        bool isDeathKnight) noexcept
    {
        bool const hasDefinition =
            snapshot.definition.has_value();

        bool const deathKnightEnabled =
            hasDefinition &&
            snapshot.definition->deathKnightEnabled;

        return ClassifyDeathKnightCreationPolicy(
            snapshot.status,
            hasDefinition,
            deathKnightEnabled,
            isDeathKnight);
    }
}

#endif
