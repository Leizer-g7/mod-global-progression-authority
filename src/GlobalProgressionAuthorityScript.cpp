#include "Runtime/ProgressionRuntime.h"
#include "Adapters/AuctionHouseBotAdapter.h"
#include "Adapters/IndividualProgressionAdapter.h"
#include "Adapters/PlayerbotsPhaseAdapter.h"

#include "Chat.h"
#include "Log.h"
#include "ScriptMgr.h"

using namespace Acore::ChatCommands;

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

                /*
                 * Pull-policy operational settings are reloadable.
                 * Authority startup settings remain intentionally
                 * unchanged until restart.
                 */
                sProgressionRuntime.ReloadPullPolicyConfig();

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

namespace
{
    char const* PersistentTransitionStatusName(
        GPA::PersistentTransitionStatus status)
    {
        using GPA::PersistentTransitionStatus;

        switch (status)
        {
            case PersistentTransitionStatus::Applied:
                return "Applied";
            case PersistentTransitionStatus::NoChange:
                return "NoChange";
            case PersistentTransitionStatus::NotInitialized:
                return "NotInitialized";
            case PersistentTransitionStatus::AuthorityUnavailable:
                return "AuthorityUnavailable";
            case PersistentTransitionStatus::InvalidTarget:
                return "InvalidTarget";
            case PersistentTransitionStatus::RollbackNotAllowed:
                return "RollbackNotAllowed";
            case PersistentTransitionStatus::RuntimePrepareFailed:
                return "RuntimePrepareFailed";
            case PersistentTransitionStatus::RuntimeApplyFailed:
                return "RuntimeApplyFailed";
            case PersistentTransitionStatus::RuntimeRecoveryFailed:
                return "RuntimeRecoveryFailed";
            case PersistentTransitionStatus::SaveFailed:
                return "SaveFailed";
            case PersistentTransitionStatus::SaveFailedRuntimeRecoveryFailed:
                return "SaveFailedRuntimeRecoveryFailed";
            case PersistentTransitionStatus::RuntimeFinalizeFailed:
                return "RuntimeFinalizeFailed";
        }

        return "Unknown";
    }

    char const* TransitionDirectionName(
        GPA::TransitionDirection direction)
    {
        using GPA::TransitionDirection;

        switch (direction)
        {
            case TransitionDirection::NoChange:
                return "NoChange";
            case TransitionDirection::Forward:
                return "Forward";
            case TransitionDirection::Rollback:
                return "Rollback";
        }

        return "Unknown";
    }

    class GlobalProgressionAuthorityCommandScript final
        : public CommandScript
    {
    public:
        GlobalProgressionAuthorityCommandScript()
            : CommandScript(
                "GlobalProgressionAuthorityCommandScript")
        {
        }

        ChatCommandTable GetCommands() const override
        {
            static ChatCommandTable gpaTable =
            {
                {
                    "status",
                    HandleStatusCommand,
                    SEC_GAMEMASTER,
                    Console::Yes
                },
                {
                    "set",
                    HandleSetCommand,
                    SEC_ADMINISTRATOR,
                    Console::Yes
                }
            };

            static ChatCommandTable commandTable =
            {
                { "gpa", gpaTable }
            };

            return commandTable;
        }

    private:
        static bool HandleStatusCommand(
            ChatHandler* handler)
        {
            GPA::RuntimeConfig const& config =
                sProgressionRuntime.GetConfig();

            std::optional<GPA::ProgressionState> const state =
                sProgressionRuntime
                    .GetManager()
                    .GetActiveState();

            GPA::PhaseDefinition const* definition =
                sProgressionRuntime
                    .GetManager()
                    .GetActiveDefinition();

            handler->SendSysMessage(
                "========== GPA STATUS ==========");

            handler->PSendSysMessage(
                "Config Loaded: {}",
                sProgressionRuntime.IsConfigLoaded()
                    ? "YES"
                    : "NO");

            handler->PSendSysMessage(
                "Enabled: {}",
                config.enabled
                    ? "YES"
                    : "NO");

            handler->PSendSysMessage(
                "Config Valid: {}",
                config.valid
                    ? "YES"
                    : "NO");

            handler->PSendSysMessage(
                "Authority Ready: {}",
                sProgressionRuntime.IsAuthorityReady()
                    ? "YES"
                    : "NO");

            handler->PSendSysMessage(
                "Rollback: {}",
                sProgressionRuntime
                        .GetManager()
                        .IsRollbackAllowed()
                    ? "ENABLED"
                    : "DISABLED");

            if (!state || !definition)
            {
                handler->SendSysMessage(
                    "Active authoritative state: UNAVAILABLE");

                handler->SendSysMessage(
                    "================================");

                return true;
            }

            handler->PSendSysMessage(
                "Phase: {}",
                static_cast<uint32>(
                    state->phase));

            handler->PSendSysMessage(
                "ContentStage: {} (allowed {}..{})",
                static_cast<uint32>(
                    state->contentStage),
                static_cast<uint32>(
                    definition->contentStageMin),
                static_cast<uint32>(
                    definition->contentStageMax));

            handler->PSendSysMessage(
                "Player Max Level: {}",
                static_cast<uint32>(
                    definition->playerMaxLevel));

            handler->PSendSysMessage(
                "Professions: Crafting={} Gathering={} Secondary={} Riding={}",
                static_cast<uint32>(
                    definition->professions.craftingCap),
                static_cast<uint32>(
                    definition->professions.gatheringCap),
                static_cast<uint32>(
                    definition->professions.secondaryCap),
                static_cast<uint32>(
                    definition->professions.ridingCap));

            handler->PSendSysMessage(
                "Outland: {}",
                definition->world.outlandEnabled
                    ? "OPEN"
                    : "LOCKED");

            handler->PSendSysMessage(
                "Northrend: {}",
                definition->world.northrendEnabled
                    ? "OPEN"
                    : "LOCKED");

            handler->PSendSysMessage(
                "Death Knights: {}",
                definition->deathKnightEnabled
                    ? "OPEN"
                    : "LOCKED");

            handler->SendSysMessage(
                "================================");

            return true;
        }

        static bool HandleSetCommand(
            ChatHandler* handler,
            uint32 requestedPhase,
            uint32 requestedContentStage)
        {
            if (requestedPhase > 255 ||
                requestedContentStage > 255)
            {
                handler->SendSysMessage(
                    "Invalid GPA state: values must fit uint8.");

                return true;
            }

            GPA::ProgressionState const target{
                static_cast<std::uint8_t>(
                    requestedPhase),
                static_cast<std::uint8_t>(
                    requestedContentStage)
            };

            std::optional<GPA::ProgressionState> const before =
                sProgressionRuntime
                    .GetManager()
                    .GetActiveState();

            GPA::PersistentTransitionResult const result =
                sProgressionRuntime.TransitionTo(
                    target);

            handler->PSendSysMessage(
                "GPA transition result: {}.",
                PersistentTransitionStatusName(
                    result.status));

            handler->PSendSysMessage(
                "Direction: {}.",
                TransitionDirectionName(
                    result.transition.direction));

            if (!result.error.empty())
            {
                handler->PSendSysMessage(
                    "Error: {}",
                    result.error);
            }

            if (!result.Succeeded())
            {
                std::optional<GPA::ProgressionState> const current =
                    sProgressionRuntime
                        .GetManager()
                        .GetActiveState();

                if (current)
                {
                    handler->PSendSysMessage(
                        "GPA memory state: Phase={}, ContentStage={}.",
                        static_cast<uint32>(
                            current->phase),
                        static_cast<uint32>(
                            current->contentStage));
                }
                else
                {
                    handler->SendSysMessage(
                        "GPA memory state: UNAVAILABLE.");
                }

                handler->PSendSysMessage(
                    "Runtime consistent: {}.",
                    result.RuntimeConsistent()
                        ? "YES"
                        : "NO");

                handler->PSendSysMessage(
                    "Safe to republish authority: {}.",
                    result.SafeToRepublishAuthority()
                        ? "YES"
                        : "NO");

                handler->PSendSysMessage(
                    "Authority Ready: {}.",
                    sProgressionRuntime.IsAuthorityReady()
                        ? "YES"
                        : "NO");

                return true;
            }

            std::optional<GPA::ProgressionState> const after =
                sProgressionRuntime
                    .GetManager()
                    .GetActiveState();

            if (after)
            {
                handler->PSendSysMessage(
                    "Active GPA state: Phase={}, ContentStage={}.",
                    static_cast<uint32>(
                        after->phase),
                    static_cast<uint32>(
                        after->contentStage));
            }
            else
            {
                handler->SendSysMessage(
                    "WARNING: transition succeeded but "
                    "active GPA state is unavailable.");
            }

            return true;
        }
    };
}

void AddSC_global_progression_authority()
{
    new GlobalProgressionAuthorityWorldScript();
    new GlobalProgressionAuthorityCommandScript();
}
