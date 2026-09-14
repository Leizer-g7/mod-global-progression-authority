#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_STATE_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_STATE_H

#include <cstdint>

namespace GPA
{
    struct ProgressionState
    {
        std::uint8_t phase = 20;
        std::uint8_t contentStage = 0;

        bool operator==(ProgressionState const& other) const
        {
            return
                phase == other.phase &&
                contentStage == other.contentStage;
        }

        bool operator!=(ProgressionState const& other) const
        {
            return !(*this == other);
        }
    };
}

#endif
