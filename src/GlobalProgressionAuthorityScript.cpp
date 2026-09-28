#include "Runtime/ProgressionRuntime.h"
#include "Adapters/AuctionHouseBotAdapter.h"
#include "Adapters/IndividualProgressionAdapter.h"
#include "Adapters/PlayerbotsPhaseAdapter.h"

#include "Log.h"
#include "ScriptMgr.h"

namespace
{
    void ConfigureExternalAdapters(
        bool authorityEnabled)
    {
        bool const individualProgressionBridge =
            GPA::Adapters::
                ConfigureIndividualProgressionAdapter(
                    authorityEnabled);

        bool const auctionHouseBotBridge =
            GPA::Adapters::
                ConfigureAuctionHouseBotAdapter(
                    authorityEnabled);

        LOG_INFO(
            "module",
            "GPA: external progression adapters configured. "
            "AuthorityEnabled={}, IndividualProgressionBridge={}, "
            "AuctionHouseBotBridge={}.",
            authorityEnabled ? 1 : 0,
            individualProgressionBridge ? 1 : 0,
            auctionHouseBotBridge ? 1 : 0);
    }

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
    private:
        bool _criticalRuntimeResyncPending = false;

    public:
        GlobalProgressionAuthorityWorldScript()
            : WorldScript(
                "GlobalProgressionAuthorityWorldScript")
        {
        }

        void OnBeforeConfigLoad(bool reload) override
        {
            if (!reload ||
                !sProgressionRuntime.IsConfigLoaded())
            {
                return;
            }

            GPA::RuntimeConfig const& config =
                sProgressionRuntime.GetConfig();

            if (!config.enabled)
            {
                return;
            }

            /*
             * Config reload is synchronous, but critical participants
             * may reload their own mutable runtime later in the same
             * OnAfterConfigLoad dispatch. Unpublish GPA before that
             * window begins and only republish after a deferred
             * critical-runtime synchronization succeeds.
             */
            _criticalRuntimeResyncPending = false;

            sProgressionRuntime.MarkAuthorityUnavailable();

            LOG_INFO(
                "module",
                "GPA: global config reload beginning. "
                "Authority unpublished until critical runtime "
                "is re-synchronized.");
        }

        void OnAfterConfigLoad(bool reload) override
        {
            if (reload)
            {
                LOG_INFO(
                    "module",
                    "GPA: global config reload detected. "
                    "Startup authority settings remain unchanged.");

                if (!sProgressionRuntime.IsConfigLoaded())
                {
                    sProgressionRuntime.MarkAuthorityUnavailable();

                    LOG_ERROR(
                        "module",
                        "GPA: cannot schedule critical runtime "
                        "re-synchronization because startup "
                        "configuration is unavailable.");

                    return;
                }

                GPA::RuntimeConfig const& config =
                    sProgressionRuntime.GetConfig();

                if (!config.enabled)
                {
                    return;
                }

                if (!config.valid)
                {
                    sProgressionRuntime.MarkAuthorityUnavailable();

                    LOG_ERROR(
                        "module",
                        "GPA: cannot schedule critical runtime "
                        "re-synchronization because startup "
                        "configuration is invalid: {}",
                        config.error);

                    return;
                }

                /*
                 * Do not synchronize here. Other module callbacks,
                 * including Playerbots, may still execute later in
                 * this same config-reload dispatch and overwrite
                 * their mutable runtime configuration.
                 */
                _criticalRuntimeResyncPending = true;

                LOG_INFO(
                    "module",
                    "GPA: critical runtime re-synchronization "
                    "scheduled for the next world update.");

                return;
            }

            sProgressionRuntime.LoadStartupConfig();

            ConfigureExternalAdapters(
                sProgressionRuntime
                    .GetConfig()
                    .enabled);
        }

        void OnUpdate(uint32 /*diff*/) override
        {
            if (!_criticalRuntimeResyncPending)
            {
                return;
            }

            /*
             * Consume exactly once. A failed re-synchronization stays
             * fail-closed instead of retrying indefinitely every tick.
             * A later explicit config reload may schedule a new attempt.
             */
            _criticalRuntimeResyncPending = false;

            std::string error;

            if (!sProgressionRuntime.SynchronizeCriticalRuntime(
                    error))
            {
                LOG_ERROR(
                    "module",
                    "GPA: critical runtime re-synchronization "
                    "after config reload failed. "
                    "Authority remains unavailable. Error={}",
                    error.empty()
                        ? "<none>"
                        : error);

                return;
            }

            sProgressionRuntime.MarkAuthorityReady();

            LOG_INFO(
                "module",
                "GPA: critical runtime re-synchronized after "
                "config reload. Authority republished.");
        }

        void OnStartup() override
        {
            if (!sProgressionRuntime.IsConfigLoaded())
            {
                sProgressionRuntime.LoadStartupConfig();

                ConfigureExternalAdapters(
                    sProgressionRuntime
                        .GetConfig()
                        .enabled);
            }

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

            /*
             * Playerbots is the first critical push participant.
             *
             * Registration happens only after bootstrap has
             * established and validated the active authoritative
             * state, but before runtime synchronization and
             * authority publication.
             */
            GPA::ICriticalRuntimeParticipant* const
                playerbotsParticipant =
                    GPA::GetPlayerbotsPhaseAdapter();

            if (playerbotsParticipant)
            {
                std::string registrationError;

                if (!sProgressionRuntime
                        .RegisterCriticalRuntimeParticipant(
                            *playerbotsParticipant,
                            registrationError))
                {
                    sProgressionRuntime.MarkAuthorityUnavailable();

                    LOG_ERROR(
                        "module",
                        "GPA: failed to register Playerbots critical "
                        "runtime participant. Authority remains "
                        "unavailable. Error={}",
                        registrationError.empty()
                            ? "<none>"
                            : registrationError);

                    return;
                }

                LOG_INFO(
                    "module",
                    "GPA: critical runtime participant registered. "
                    "Name=PlayerbotsPhaseAdapter, Count={}.",
                    sProgressionRuntime
                        .GetCriticalRuntimeParticipantCount());
            }
            else
            {
                LOG_INFO(
                    "module",
                    "GPA: Playerbots integration is not available "
                    "in this build. Critical runtime participant "
                    "registration skipped.");
            }

            /*
             * Synchronize all registered critical push adapters
             * before publishing authority to pull providers.
             */
            std::string runtimeError;

            if (!sProgressionRuntime.SynchronizeCriticalRuntime(
                    runtimeError))
            {
                LOG_ERROR(
                    "module",
                    "GPA: critical runtime synchronization failed. "
                    "Authority remains unavailable. Error={}",
                    runtimeError.empty()
                        ? "<none>"
                        : runtimeError);

                return;
            }

            sProgressionRuntime.MarkAuthorityReady();

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
