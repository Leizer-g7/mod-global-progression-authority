#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_ADAPTER_POLICIES_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_ADAPTER_POLICIES_H

#include <cstdint>
#include <string>

namespace GPA
{
    /*
     * Desired Playerbots state for one GPA phase.
     *
     * These values deliberately do not belong to PhaseDefinition.
     * PlayerbotsAdapter is responsible for translating them into
     * the current Playerbots API.
     */
    struct PlayerbotsPhasePolicy
    {
        std::uint8_t maxLevel = 20;

        std::uint32_t gearMaxItemLevel = 25;

        std::string distribution =
            "1-9:15,10-19:50,20:35";
    };

    /*
     * Progression-related economy policy.
     *
     * This is intentionally independent from Playerbots gear limits.
     */
    struct EconomyPhasePolicy
    {
        std::uint32_t materialMaxItemLevel = 20;
    };

    /*
     * Desired Battleground bracket mapping for one phase.
     *
     * Whether Battleground automation itself is globally enabled is
     * adapter configuration and is not stored here.
     */
    struct BattlegroundPhasePolicy
    {
        std::string warsongGulch = "0";
        std::string arathiBasin = "0";
        std::string alteracValley = "0";
        std::string eyeOfTheStorm = "0";
        std::string isleOfConquest = "0";
    };
}

#endif
