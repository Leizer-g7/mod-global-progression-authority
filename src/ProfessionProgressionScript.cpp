#include "ProfessionPullPolicy.h"

#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace
{
    thread_local bool gProfessionClampInProgress = false;

    class ProfessionClampGuard
    {
    public:
        ProfessionClampGuard()
        {
            gProfessionClampInProgress = true;
        }

        ~ProfessionClampGuard()
        {
            gProfessionClampInProgress = false;
        }
    };

    constexpr std::array<std::uint32_t, 15>
        ProfessionSkills =
    {
        129, // First Aid
        185, // Cooking
        356, // Fishing

        164, // Blacksmithing
        165, // Leatherworking
        171, // Alchemy
        197, // Tailoring
        202, // Engineering
        333, // Enchanting
        755, // Jewelcrafting
        773, // Inscription

        182, // Herbalism
        186, // Mining
        393, // Skinning

        762  // Riding
    };

    GPA::ProfessionPullDecision GetProfessionDecision(
        GPA::PullPolicySnapshot const& snapshot,
        Player const* player,
        std::uint32_t skillId)
    {
        return GPA::ClassifyProfessionPullPolicy(
            snapshot,
            player && player->IsGameMaster(),
            skillId);
    }

    void ClampProfessionSkill(
        Player* player,
        GPA::PullPolicySnapshot const& snapshot,
        std::uint32_t skillId)
    {
        if (!player ||
            gProfessionClampInProgress)
        {
            return;
        }

        GPA::ProfessionPullDecision const decision =
            GetProfessionDecision(
                snapshot,
                player,
                skillId);

        if (!decision.managed ||
            decision.mode !=
                GPA::PlayerPullPolicyMode::EnforceKnownCap ||
            !decision.cap ||
            !player->HasSkill(skillId))
        {
            return;
        }

        std::uint16_t const currentValue =
            player->GetPureSkillValue(skillId);

        /*
         * OnPlayerGetMaxSkillValue already exposes the
         * authoritative cap.
         */
        std::uint16_t const exposedMax =
            player->GetPureMaxSkillValue(skillId);

        std::uint16_t const targetMax =
            std::min<std::uint16_t>(
                exposedMax,
                decision.cap);

        if (!targetMax)
            return;

        std::uint16_t const targetValue =
            std::min<std::uint16_t>(
                currentValue,
                targetMax);

        std::uint16_t const currentStep =
            player->GetSkillStep(
                static_cast<std::uint16_t>(
                    skillId));

        std::uint16_t targetStep =
            std::min<std::uint16_t>(
                currentStep,
                GPA::GetMaxAllowedProfessionStep(
                    decision.cap));

        if (!targetStep)
            targetStep = 1;

        ProfessionClampGuard guard;

        player->SetSkill(
            static_cast<std::uint16_t>(skillId),
            targetStep,
            targetValue,
            targetMax);
    }

    void ClampAllProfessionSkills(
        Player* player,
        GPA::PullPolicySnapshot const& snapshot)
    {
        if (!player)
            return;

        for (std::uint32_t skillId :
             ProfessionSkills)
        {
            ClampProfessionSkill(
                player,
                snapshot,
                skillId);
        }
    }

    bool SpellViolatesProfessionPolicy(
        Player const* player,
        GPA::PullPolicySnapshot const& snapshot,
        std::uint32_t spellId,
        std::uint32_t depth)
    {
        if (!player ||
            !spellId ||
            depth > 4)
        {
            return false;
        }

        auto violatesSkill =
            [&](std::uint32_t skillId,
                std::uint32_t newMax,
                std::uint32_t newStep)
            {
                GPA::ProfessionPullDecision const decision =
                    GetProfessionDecision(
                        snapshot,
                        player,
                        skillId);

                if (!decision.managed ||
                    decision.mode ==
                        GPA::PlayerPullPolicyMode::Bypass)
                {
                    return false;
                }

                if (decision.mode ==
                    GPA::PlayerPullPolicyMode::BlockProgression)
                {
                    return true;
                }

                if (!decision.cap)
                    return true;

                if (newMax > decision.cap)
                    return true;

                return newStep >
                    GPA::GetMaxAllowedProfessionStep(
                        decision.cap);
            };

        if (SpellLearnSkillNode const* learnSkill =
                sSpellMgr->GetSpellLearnSkill(spellId))
        {
            std::uint32_t const newMax =
                learnSkill->maxvalue
                    ? learnSkill->maxvalue
                    : player->GetMaxSkillValueForLevel();

            if (violatesSkill(
                    learnSkill->skill,
                    newMax,
                    learnSkill->step))
            {
                return true;
            }
        }

        SpellInfo const* spellInfo =
            sSpellMgr->GetSpellInfo(spellId);

        if (!spellInfo)
            return false;

        for (SpellEffectInfo const& effect :
             spellInfo->GetEffects())
        {
            if (effect.IsEffect(
                    SPELL_EFFECT_SKILL_STEP))
            {
                std::uint32_t const skillId =
                    static_cast<std::uint32_t>(
                        effect.MiscValue);

                std::int32_t const step =
                    effect.CalcValue();

                if (step > 0)
                {
                    std::uint32_t const newStep =
                        static_cast<std::uint32_t>(
                            step);

                    std::uint32_t const newMax =
                        newStep * 75;

                    if (violatesSkill(
                            skillId,
                            newMax,
                            newStep))
                    {
                        return true;
                    }
                }
            }

            if (effect.IsEffect(
                    SPELL_EFFECT_LEARN_SPELL) &&
                effect.TriggerSpell)
            {
                if (SpellViolatesProfessionPolicy(
                        player,
                        snapshot,
                        effect.TriggerSpell,
                        depth + 1))
                {
                    return true;
                }
            }
        }

        return false;
    }

    class GlobalProgressionProfessionScript final
        : public PlayerScript
    {
    public:
        GlobalProgressionProfessionScript()
            : PlayerScript(
                "GlobalProgressionProfessionScript")
        {
        }

        void OnPlayerGetMaxSkillValue(
            Player* player,
            uint32 skill,
            int32& result,
            bool /*isPure*/) override
        {
            if (!player)
                return;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::ProfessionPullDecision const decision =
                GetProfessionDecision(
                    snapshot,
                    player,
                    skill);

            if (!decision.managed ||
                decision.mode ==
                    GPA::PlayerPullPolicyMode::Bypass)
            {
                return;
            }

            if (decision.mode ==
                    GPA::PlayerPullPolicyMode::EnforceKnownCap &&
                decision.cap)
            {
                if (result > decision.cap)
                    result = decision.cap;

                return;
            }

            /*
             * GPA is active but there is no usable authoritative
             * phase definition.
             *
             * Do not invent a cap and do not reduce an existing
             * skill. Expose no growth room beyond the current
             * pure value until authority returns.
             */
            int32 const currentValue =
                static_cast<int32>(
                    player->GetPureSkillValue(skill));

            if (result > currentValue)
                result = currentValue;
        }

        bool OnPlayerCanUpdateSkill(
            Player* player,
            uint32 skillId) override
        {
            if (!player)
                return true;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::ProfessionPullDecision const decision =
                GetProfessionDecision(
                    snapshot,
                    player,
                    skillId);

            if (!decision.managed ||
                decision.mode ==
                    GPA::PlayerPullPolicyMode::Bypass)
            {
                return true;
            }

            if (decision.mode ==
                    GPA::PlayerPullPolicyMode::EnforceKnownCap &&
                decision.cap)
            {
                return player->GetSkillValue(skillId) <
                       decision.cap;
            }

            return false;
        }

        void OnPlayerLogin(Player* player) override
        {
            if (!player)
                return;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            /*
             * Clamp only when an authoritative definition exists.
             * In Unavailable fail-closed mode we intentionally do
             * not guess a destructive cap.
             */
            ClampAllProfessionSkills(
                player,
                snapshot);
        }

        void OnPlayerSetSkill(
            Player* player,
            uint32 skillId,
            uint32 value,
            uint32 max,
            uint32 /*step*/,
            uint32 newValue) override
        {
            if (!player ||
                gProfessionClampInProgress)
            {
                return;
            }

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::ProfessionPullDecision const decision =
                GetProfessionDecision(
                    snapshot,
                    player,
                    skillId);

            if (!decision.managed ||
                decision.mode ==
                    GPA::PlayerPullPolicyMode::Bypass)
            {
                return;
            }

            if (decision.mode ==
                    GPA::PlayerPullPolicyMode::EnforceKnownCap &&
                decision.cap)
            {
                std::uint16_t const currentStep =
                    player->GetSkillStep(
                        static_cast<std::uint16_t>(
                            skillId));

                if (newValue > decision.cap ||
                    max > decision.cap ||
                    currentStep >
                        GPA::GetMaxAllowedProfessionStep(
                            decision.cap))
                {
                    ClampProfessionSkill(
                        player,
                        snapshot,
                        skillId);
                }

                return;
            }

            /*
             * Unknown-authority fail-closed path.
             *
             * OnPlayerSetSkill is post-mutation and does not expose
             * the previous max/step. Never guess them and never
             * downgrade valid historical state.
             *
             * We can, however, safely undo an increase in the raw
             * skill value because the hook gives us its previous
             * value explicitly.
             */
            if (newValue > value)
            {
                std::uint16_t const currentStep =
                    player->GetSkillStep(
                        static_cast<std::uint16_t>(
                            skillId));

                ProfessionClampGuard guard;

                player->SetSkill(
                    static_cast<std::uint16_t>(
                        skillId),
                    currentStep,
                    static_cast<std::uint16_t>(
                        value),
                    static_cast<std::uint16_t>(
                        max));
            }
        }

        bool OnPlayerCanLearnTrainerSpell(
            Player const* player,
            uint32 spellId) override
        {
            if (!player)
                return true;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            return !SpellViolatesProfessionPolicy(
                player,
                snapshot,
                spellId,
                0);
        }
    };
}

void AddSC_global_progression_profession_policy()
{
    new GlobalProgressionProfessionScript();
}
