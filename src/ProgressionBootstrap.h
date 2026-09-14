#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_BOOTSTRAP_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_BOOTSTRAP_H

#include "ProgressionMgr.h"
#include "ProgressionStateStore.h"

#include <string>

namespace GPA
{
    enum class BootstrapStatus
    {
        LoadedPersistedState,
        CreatedDefaultState,

        AlreadyInitialized,
        LoadError,
        MalformedLoadResult,
        InvalidPersistedState,
        InvalidDefaultState,
        SaveDefaultFailed,
        ManagerInitializationFailed
    };

    struct BootstrapResult
    {
        BootstrapStatus status =
            BootstrapStatus::LoadError;

        std::string error;

        bool Succeeded() const
        {
            return
                status ==
                    BootstrapStatus::LoadedPersistedState ||
                status ==
                    BootstrapStatus::CreatedDefaultState;
        }
    };

    class ProgressionBootstrap
    {
    public:
        static BootstrapResult Initialize(
            ProgressionMgr& manager,
            IProgressionStateStore& store,
            ProgressionState const& defaultState,
            bool allowRollback);
    };
}

#endif
