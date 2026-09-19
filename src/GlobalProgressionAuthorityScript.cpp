#include "Runtime/ProgressionRuntime.h"

#include "Log.h"
#include "ScriptMgr.h"

namespace
{
    char const* BootstrapStatusName(
        GPA::BootstrapStatus status)
    {
        using GPA::BootstrapStatus;

        switch (status)
        {
            case BootstrapStatus::LoadedPersistedState:
                return "LoadedPersistedState";
            case BootstrapStatus::CreatedDefaultState:
                return "CreatedDefaultState";
            case BootstrapStatus::AlreadyInitialized:
                return "AlreadyInitialized";
            case BootstrapStatus::LoadError:
                return "LoadError";
            case BootstrapStatus::MalformedLoadResult:
                return "MalformedLoadResult";
            case BootstrapStatus::InvalidPersistedState:
                return "InvalidPersistedState";
            case BootstrapStatus::InvalidDefaultState:
                return "InvalidDefaultState";
            case BootstrapStatus::SaveDefaultFailed:
                return "SaveDefaultFailed";
            case BootstrapStatus::ManagerInitializationFailed:
                return "ManagerInitializationFailed";
        }

        return "Unknown";
    }

    class GlobalProgressionAuthorityWorldScript final
        : public WorldScript
    {
    public:
        GlobalProgressionAuthorityWorldScript()
            : WorldScript(
                "GlobalProgressionAuthorityWorldScript")
        {
        }

        void OnAfterConfigLoad(bool reload) override
        {
            if (reload)
            {
                LOG_INFO(
                    "module",
                    "GPA: global config reload detected. "
                    "Startup authority settings remain unchanged "
                    "until the next worldserver start.");

                return;
            }

            sProgressionRuntime.LoadStartupConfig();
        }

        void OnStartup() override
        {
            if (!sProgressionRuntime.IsConfigLoaded())
                sProgressionRuntime.LoadStartupConfig();

            GPA::RuntimeConfig const& config =
                sProgressionRuntime.GetConfig();

            if (!config.enabled)
            {
                LOG_INFO(
                    "module",
                    "GPA: runtime authority disabled.");

                return;
            }

            if (!config.valid)
            {
                LOG_ERROR(
                    "module",
                    "GPA: bootstrap not attempted because "
                    "startup configuration is invalid: {}",
                    config.error);

                return;
            }

            GPA::BootstrapResult const result =
                sProgressionRuntime.Bootstrap();

            if (!result.Succeeded())
            {
                LOG_ERROR(
                    "module",
                    "GPA: bootstrap failed. Status={}, Error={}",
                    BootstrapStatusName(result.status),
                    result.error.empty()
                        ? "<none>"
                        : result.error);

                return;
            }

            std::optional<GPA::ProgressionState> const active =
                sProgressionRuntime
                    .GetManager()
                    .GetActiveState();

            if (!active)
            {
                LOG_ERROR(
                    "module",
                    "GPA: bootstrap reported success but "
                    "no active state was published.");

                return;
            }

            LOG_INFO(
                "module",
                "GPA: global progression authority initialized. "
                "Status={}, Phase={}, ContentStage={}, "
                "Rollback={}.",
                BootstrapStatusName(result.status),
                static_cast<unsigned>(active->phase),
                static_cast<unsigned>(active->contentStage),
                sProgressionRuntime
                        .GetManager()
                        .IsRollbackAllowed()
                    ? 1
                    : 0);
        }
    };
}

void AddSC_global_progression_authority()
{
    new GlobalProgressionAuthorityWorldScript();
}
