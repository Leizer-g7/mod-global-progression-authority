#include "PlayerPullPolicy.h"

#include "Pet.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "World.h"

#include <algorithm>

namespace
{
    GPA::PlayerPullPolicyMode GetPlayerPolicyMode(
        GPA::PullPolicySnapshot const& snapshot,
        Player const* player)
    {
        return GPA::ClassifyPlayerPullPolicy(
            snapshot,
            player && player->IsGameMaster());
    }

    void PreserveKillPetXP(
        Player* player,
        uint32 amount,
        uint8 xpSource)
    {
        if (!player ||
            !amount ||
            xpSource != PlayerXPSource::XPSOURCE_KILL)
        {
            return;
        }

        if (Pet* pet = player->GetPet())
        {
            pet->GivePetXP(
                player->GetGroup()
                    ? amount / 2
                    : amount);
        }
    }

    class GlobalProgressionPlayerScript final
        : public PlayerScript
    {
    public:
        GlobalProgressionPlayerScript()
            : PlayerScript("GlobalProgressionPlayerScript")
        {
        }

        void OnPlayerSetMaxLevel(
            Player* player,
            uint32& maxPlayerLevel) override
        {
            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::PlayerPullPolicyMode const mode =
                GetPlayerPolicyMode(snapshot, player);

            if (mode == GPA::PlayerPullPolicyMode::Bypass)
                return;

            if (mode ==
                    GPA::PlayerPullPolicyMode::EnforceKnownCap &&
                snapshot.definition)
            {
                maxPlayerLevel =
                    std::min<uint32>(
                        maxPlayerLevel,
                        snapshot.definition->playerMaxLevel);

                return;
            }

            /*
             * GPA is enabled but no authoritative definition is
             * currently available.
             *
             * Do not invent a phase cap. Freeze progression at the
             * player's current level until authority is available.
             */
            if (player)
            {
                maxPlayerLevel =
                    std::min<uint32>(
                        maxPlayerLevel,
                        player->GetLevel());
            }
        }

        bool OnPlayerCanGiveLevel(
            Player* player,
            uint8 newLevel) override
        {
            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::PlayerPullPolicyMode const mode =
                GetPlayerPolicyMode(snapshot, player);

            if (mode == GPA::PlayerPullPolicyMode::Bypass)
                return true;

            if (mode ==
                    GPA::PlayerPullPolicyMode::EnforceKnownCap &&
                snapshot.definition)
            {
                return newLevel <=
                    snapshot.definition->playerMaxLevel;
            }

            return false;
        }

        void OnPlayerGiveXP(
            Player* player,
            uint32& amount,
            Unit* /*victim*/,
            uint8 xpSource) override
        {
            if (!player || !amount)
                return;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::PlayerPullPolicyMode const mode =
                GetPlayerPolicyMode(snapshot, player);

            if (mode == GPA::PlayerPullPolicyMode::Bypass)
                return;

            bool blockXP = false;

            if (mode ==
                    GPA::PlayerPullPolicyMode::EnforceKnownCap &&
                snapshot.definition)
            {
                blockXP =
                    player->GetLevel() >=
                    snapshot.definition->playerMaxLevel;
            }
            else
            {
                /*
                 * Active GPA without a usable authoritative
                 * definition is fail-closed for player progression.
                 */
                blockXP = true;
            }

            if (!blockXP)
                return;

            /*
             * KillRewarder gives pet XP after this hook using the
             * possibly modified player XP amount. Preserve the pet
             * reward before suppressing player XP.
             */
            PreserveKillPetXP(
                player,
                amount,
                xpSource);

            amount = 0;
        }

        bool OnPlayerShouldBeRewardedWithMoneyInsteadOfExp(
            Player* player) override
        {
            if (!player)
                return false;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::PlayerPullPolicyMode const mode =
                GetPlayerPolicyMode(snapshot, player);

            if (mode !=
                    GPA::PlayerPullPolicyMode::EnforceKnownCap ||
                !snapshot.definition)
            {
                return false;
            }

            return player->GetLevel() >=
                snapshot.definition->playerMaxLevel;
        }

        void OnPlayerQuestComputeMaxLevelMoney(
            Player* player,
            Quest const* quest,
            uint32& moneyValue) override
        {
            if (!player || !quest)
                return;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            GPA::PlayerPullPolicyMode const mode =
                GetPlayerPolicyMode(snapshot, player);

            if (mode !=
                    GPA::PlayerPullPolicyMode::EnforceKnownCap ||
                !snapshot.definition ||
                player->GetLevel() <
                    snapshot.definition->playerMaxLevel)
            {
                return;
            }

            if (quest->HasFlag(
                    QUEST_FLAGS_NO_MONEY_FROM_XP))
            {
                moneyValue = 0;
                return;
            }

            uint32 const xpValue =
                quest->XPValue(
                    snapshot.definition->playerMaxLevel);

            moneyValue =
                static_cast<uint32>(
                    (xpValue * (6 * COPPER)) *
                    sWorld->getRate(
                        RATE_REWARD_BONUS_MONEY));
        }
    };
}

void AddSC_global_progression_player_policy()
{
    new GlobalProgressionPlayerScript();
}
