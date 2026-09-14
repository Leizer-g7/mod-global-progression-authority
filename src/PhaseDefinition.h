#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PHASE_DEFINITION_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PHASE_DEFINITION_H

#include <cstdint>

namespace GPA
{
    struct ProfessionPolicy
    {
        std::uint16_t craftingCap = 150;
        std::uint16_t gatheringCap = 150;
        std::uint16_t secondaryCap = 150;
        std::uint16_t ridingCap = 75;
    };

    struct WorldAvailability
    {
        bool outlandEnabled = false;
        bool northrendEnabled = false;
    };

    struct PhaseDefinition
    {
        std::uint8_t phase = 20;

        std::uint8_t contentStageMin = 0;
        std::uint8_t contentStageMax = 0;

        std::uint8_t playerMaxLevel = 20;

        ProfessionPolicy professions;
        WorldAvailability world;

        bool deathKnightEnabled = false;
    };
}

#endif
