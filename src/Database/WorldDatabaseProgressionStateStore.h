#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_WORLD_DATABASE_PROGRESSION_STATE_STORE_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_WORLD_DATABASE_PROGRESSION_STATE_STORE_H

#include "ProgressionStateStore.h"

namespace GPA
{
    class WorldDatabaseProgressionStateStore final
        : public IProgressionStateStore
    {
    public:
        StateLoadResult Load() override;

        bool Save(
            ProgressionState const& state,
            std::string& error) override;
    };
}

#endif
