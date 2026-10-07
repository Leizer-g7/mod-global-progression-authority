#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROFESSION_PULL_POLICY_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROFESSION_PULL_POLICY_H

#include "PhaseDefinition.h"
#include "PlayerPullPolicy.h"

#include <algorithm>
#include <cstdint>

namespace GPA
{
    enum class ProfessionSkillKind : std::uint8_t
    {
        None,
        Crafting,
        Gathering,
        Secondary,
        Riding
    };

    struct ProfessionPullDecision
    {
        PlayerPullPolicyMode mode =
            PlayerPullPolicyMode::Bypass;

        bool managed = false;

        std::uint16_t cap = 0;
    };

    inline ProfessionSkillKind ClassifyProfessionSkill(
        std::uint32_t skillId)
    {
        switch (skillId)
        {
            // Crafting
            case 164: // Blacksmithing
            case 165: // Leatherworking
            case 171: // Alchemy
            case 197: // Tailoring
            case 202: // Engineering
            case 333: // Enchanting
            case 755: // Jewelcrafting
            case 773: // Inscription
                return ProfessionSkillKind::Crafting;

            // Gathering
            case 182: // Herbalism
            case 186: // Mining
            case 393: // Skinning
                return ProfessionSkillKind::Gathering;

            // Secondary
            case 129: // First Aid
            case 185: // Cooking
            case 356: // Fishing
                return ProfessionSkillKind::Secondary;

            // Riding
            case 762:
                return ProfessionSkillKind::Riding;

            default:
                return ProfessionSkillKind::None;
        }
    }

    inline bool IsManagedProfessionSkill(
        std::uint32_t skillId)
    {
        return ClassifyProfessionSkill(skillId) !=
               ProfessionSkillKind::None;
    }

    inline std::uint16_t GetProfessionCap(
        PhaseDefinition const& definition,
        std::uint32_t skillId)
    {
        switch (ClassifyProfessionSkill(skillId))
        {
            case ProfessionSkillKind::Crafting:
                return definition.professions.craftingCap;

            case ProfessionSkillKind::Gathering:
                return definition.professions.gatheringCap;

            case ProfessionSkillKind::Secondary:
                return definition.professions.secondaryCap;

            case ProfessionSkillKind::Riding:
                return definition.professions.ridingCap;

            case ProfessionSkillKind::None:
                return 0;
        }

        return 0;
    }

    inline std::uint16_t GetMaxAllowedProfessionStep(
        std::uint16_t cap)
    {
        if (!cap)
            return 0;

        return std::max<std::uint16_t>(
            1,
            cap / 75);
    }

    inline ProfessionPullDecision BuildProfessionPullDecision(
        PlayerPullPolicyMode mode,
        PhaseDefinition const* definition,
        std::uint32_t skillId)
    {
        ProfessionPullDecision decision;

        decision.mode = mode;
        decision.managed =
            IsManagedProfessionSkill(skillId);

        if (!decision.managed)
            return decision;

        if (mode ==
                PlayerPullPolicyMode::EnforceKnownCap &&
            definition)
        {
            decision.cap =
                GetProfessionCap(
                    *definition,
                    skillId);

            return decision;
        }

        /*
         * EnforceKnownCap without a definition must never
         * silently become permissive.
         */
        if (mode ==
                PlayerPullPolicyMode::EnforceKnownCap &&
            !definition)
        {
            decision.mode =
                PlayerPullPolicyMode::BlockProgression;
        }

        return decision;
    }

    inline ProfessionPullDecision ClassifyProfessionPullPolicy(
        PullPolicySnapshot const& snapshot,
        bool isGameMaster,
        std::uint32_t skillId)
    {
        PlayerPullPolicyMode const mode =
            ClassifyPlayerPullPolicy(
                snapshot,
                isGameMaster);

        PhaseDefinition const* definition =
            snapshot.definition
                ? &*snapshot.definition
                : nullptr;

        return BuildProfessionPullDecision(
            mode,
            definition,
            skillId);
    }
}

#endif
