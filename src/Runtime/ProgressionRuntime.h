#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_RUNTIME_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_RUNTIME_H

#include "Database/WorldDatabaseProgressionStateStore.h"
#include "ProgressionBootstrap.h"
#include "ProgressionMgr.h"
#include "ProgressionState.h"

#include <atomic>
#include <string>

namespace GPA
{
    struct RuntimeConfig
    {
        bool enabled = false;

        ProgressionState defaultState{
            20,
            0
        };

        bool allowRollback = false;

        bool valid = true;
        std::string error;
    };

    class ProgressionRuntime
    {
    public:
        static ProgressionRuntime& Instance();

        void LoadStartupConfig();

        bool IsConfigLoaded() const;

        RuntimeConfig const& GetConfig() const;

        ProgressionMgr& GetManager();

        BootstrapResult Bootstrap();

        bool IsAuthorityReady() const;

        void MarkAuthorityReady();

        void MarkAuthorityUnavailable();

    private:
        ProgressionRuntime() = default;

        RuntimeConfig _config;
        bool _configLoaded = false;

        std::atomic<bool> _authorityReady{false};

        ProgressionMgr _manager;
        WorldDatabaseProgressionStateStore _store;
    };
}

#define sProgressionRuntime GPA::ProgressionRuntime::Instance()

#endif
