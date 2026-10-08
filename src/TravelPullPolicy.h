#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_TRAVEL_PULL_POLICY_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_TRAVEL_PULL_POLICY_H

#include "PlayerPullPolicy.h"

#include <cstdint>

namespace GPA
{
    enum class TravelExpansion
    {
        None,
        Outland,
        Northrend
    };

    enum class TravelRestriction
    {
        None,
        OutlandPhase,
        OutlandLevel,
        NorthrendPhase,
        NorthrendLevel,
        AuthorityUnavailable
    };

    struct TravelTarget
    {
        TravelExpansion expansion = TravelExpansion::None;

        /*
         * true solamente para el continente directo:
         *   Outland   map 530
         *   Northrend map 571
         *
         * Las instancias que heredan Parent=530/571 utilizan
         * directContinent=false y conservan sus propios requisitos
         * de nivel del core.
         */
        bool directContinent = false;

        /*
         * Las zonas iniciales Blood Elf/Draenei están físicamente
         * en map 530 pero nunca obedecen el gate global de Outland.
         */
        bool starterZone = false;
    };

    struct TravelPullDecision
    {
        PlayerPullPolicyMode mode =
            PlayerPullPolicyMode::Bypass;

        TravelRestriction restriction =
            TravelRestriction::None;

        bool IsBlocked() const
        {
            return restriction != TravelRestriction::None;
        }
    };

    inline TravelPullDecision BuildTravelPullDecision(
        PlayerPullPolicyMode mode,
        PhaseDefinition const* definition,
        TravelTarget const& target,
        std::uint8_t playerLevel)
    {
        TravelPullDecision decision;
        decision.mode = mode;

        /*
         * Naxxramas and every non-expansion map arrive here as None.
         */
        if (target.expansion == TravelExpansion::None)
            return decision;

        /*
         * Structural exception independent of progression state.
         */
        if (target.starterZone)
            return decision;

        if (mode == PlayerPullPolicyMode::Bypass)
            return decision;

        /*
         * Unknown authority state: fail closed for managed
         * expansion content, without inventing a phase.
         */
        if (mode == PlayerPullPolicyMode::BlockProgression ||
            !definition)
        {
            decision.restriction =
                TravelRestriction::AuthorityUnavailable;

            return decision;
        }

        if (target.expansion == TravelExpansion::Outland)
        {
            if (!definition->world.outlandEnabled)
            {
                decision.restriction =
                    TravelRestriction::OutlandPhase;

                return decision;
            }

            if (target.directContinent &&
                playerLevel < 58)
            {
                decision.restriction =
                    TravelRestriction::OutlandLevel;

                return decision;
            }

            return decision;
        }

        if (target.expansion == TravelExpansion::Northrend)
        {
            if (!definition->world.northrendEnabled)
            {
                decision.restriction =
                    TravelRestriction::NorthrendPhase;

                return decision;
            }

            if (target.directContinent &&
                playerLevel < 68)
            {
                decision.restriction =
                    TravelRestriction::NorthrendLevel;

                return decision;
            }

            return decision;
        }

        return decision;
    }

    inline TravelPullDecision ClassifyTravelPullPolicy(
        PullPolicySnapshot const& snapshot,
        bool isGameMaster,
        TravelTarget const& target,
        std::uint8_t playerLevel)
    {
        PlayerPullPolicyMode const mode =
            ClassifyPlayerPullPolicy(
                snapshot,
                isGameMaster);

        PhaseDefinition const* definition = nullptr;

        if (snapshot.definition)
            definition = &*snapshot.definition;

        return BuildTravelPullDecision(
            mode,
            definition,
            target,
            playerLevel);
    }
}

#endif
