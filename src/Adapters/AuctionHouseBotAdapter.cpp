#include "AuctionHouseBotAdapter.h"

#include "../EconomyPolicyCatalog.h"
#include "../Runtime/ProgressionRuntime.h"

#if __has_include("../../../mod-ah-bot-plus/src/AuctionHouseBotProgressionProvider.h")
#include "../../../mod-ah-bot-plus/src/AuctionHouseBotProgressionProvider.h"
#define GPA_HAS_AHBOT_PROGRESSION_BRIDGE 1

#if defined(__GNUC__) || defined(__clang__)
namespace AuctionHouseBotProgression
{
    void RegisterPolicyProvider(
        PolicyProvider provider)
        __attribute__((weak));
}
#endif

#else
#define GPA_HAS_AHBOT_PROGRESSION_BRIDGE 0
#endif

namespace GPA::Adapters
{
    namespace
    {
#if GPA_HAS_AHBOT_PROGRESSION_BRIDGE
        bool ProvidePolicy(
            AuctionHouseBotProgression::Policy& policy)
        {
            GPA::PhaseDefinition const* definition =
                sProgressionRuntime
                    .GetManager()
                    .GetActiveDefinition();

            if (!definition)
                return false;

            static GPA::EconomyPolicyCatalog const
                economyCatalog;

            GPA::EconomyPhasePolicy const*
                economyPolicy =
                    economyCatalog.Find(
                        definition->phase);

            if (!economyPolicy)
                return false;

            policy.phase =
                definition->phase;

            policy.maxPlayerLevel =
                definition->playerMaxLevel;

            policy.auctionMaterialMaxItemLevel =
                economyPolicy->materialMaxItemLevel;

            policy.craftingCap =
                definition->professions.craftingCap;

            policy.gatheringCap =
                definition->professions.gatheringCap;

            policy.secondaryCap =
                definition->professions.secondaryCap;

            policy.ridingCap =
                definition->professions.ridingCap;

            return true;
        }
#endif
    }

    bool ConfigureAuctionHouseBotAdapter(
        bool enabled)
    {
#if GPA_HAS_AHBOT_PROGRESSION_BRIDGE
        auto registerProvider =
            &AuctionHouseBotProgression::
                RegisterPolicyProvider;

#if defined(__GNUC__) || defined(__clang__)
        if (!registerProvider)
            return false;
#endif

        registerProvider(
            enabled
                ? &ProvidePolicy
                : nullptr);

        return true;
#else
        (void)enabled;
        return false;
#endif
    }
}
