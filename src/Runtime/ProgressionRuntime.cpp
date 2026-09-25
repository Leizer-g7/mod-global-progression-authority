#include "ProgressionRuntime.h"

#include "Config.h"
#include "Log.h"

#include <cstdint>
#include <limits>

namespace GPA
{
    ProgressionRuntime::ProgressionRuntime()
        : _transitionCoordinator(
              _manager,
              _store,
              _criticalRuntime)
    {
    }

    ProgressionRuntime& ProgressionRuntime::Instance()
    {
        static ProgressionRuntime instance;
        return instance;
    }

    void ProgressionRuntime::LoadStartupConfig()
    {
        RuntimeConfig config;

        config.enabled =
            sConfigMgr->GetOption<bool>(
                "GlobalProgression.Enable",
                false);

        config.allowRollback =
            sConfigMgr->GetOption<bool>(
                "GlobalProgression.AllowRollback",
                false);

        std::uint32_t const rawPhase =
            sConfigMgr->GetOption<std::uint32_t>(
                "GlobalProgression.DefaultPhase",
                20);

        std::uint32_t const rawContentStage =
            sConfigMgr->GetOption<std::uint32_t>(
                "GlobalProgression.DefaultContentStage",
                0);

        constexpr std::uint32_t maxStateValue =
            std::numeric_limits<std::uint8_t>::max();

        if (rawPhase > maxStateValue)
        {
            config.valid = false;
            config.error =
                "GlobalProgression.DefaultPhase exceeds "
                "the supported uint8 range.";
        }
        else if (rawContentStage > maxStateValue)
        {
            config.valid = false;
            config.error =
                "GlobalProgression.DefaultContentStage exceeds "
                "the supported uint8 range.";
        }
        else
        {
            config.defaultState = {
                static_cast<std::uint8_t>(rawPhase),
                static_cast<std::uint8_t>(rawContentStage)
            };
        }

        _config = config;
        _configLoaded = true;

        LOG_INFO(
            "module",
            "GPA: startup configuration loaded. "
            "Enabled={}, DefaultPhase={}, "
            "DefaultContentStage={}, AllowRollback={}, Valid={}.",
            _config.enabled ? 1 : 0,
            rawPhase,
            rawContentStage,
            _config.allowRollback ? 1 : 0,
            _config.valid ? 1 : 0);

        if (!_config.valid)
        {
            LOG_ERROR(
                "module",
                "GPA: invalid startup configuration: {}",
                _config.error);
        }
    }

    bool ProgressionRuntime::IsConfigLoaded() const
    {
        return _configLoaded;
    }

    RuntimeConfig const&
    ProgressionRuntime::GetConfig() const
    {
        return _config;
    }

    ProgressionMgr& ProgressionRuntime::GetManager()
    {
        return _manager;
    }

    bool ProgressionRuntime::RegisterCriticalRuntimeParticipant(
        ICriticalRuntimeParticipant& participant,
        std::string& error)
    {
        return _criticalRuntime.RegisterParticipant(
            participant,
            error);
    }

    std::size_t
    ProgressionRuntime::GetCriticalRuntimeParticipantCount() const
    {
        return _criticalRuntime.ParticipantCount();
    }

    bool ProgressionRuntime::IsAuthorityReady() const
    {
        return _authorityReady.load(
            std::memory_order_acquire);
    }

    void ProgressionRuntime::MarkAuthorityReady()
    {
        _authorityReady.store(
            true,
            std::memory_order_release);
    }

    void ProgressionRuntime::MarkAuthorityUnavailable()
    {
        _authorityReady.store(
            false,
            std::memory_order_release);
    }

    bool ProgressionRuntime::TryBeginAuthorityTransition()
    {
        bool expected = true;

        return _authorityReady.compare_exchange_strong(
            expected,
            false,
            std::memory_order_acq_rel,
            std::memory_order_acquire);
    }

    BootstrapResult ProgressionRuntime::Bootstrap()
    {
        /*
         * Bootstrap only establishes the authoritative state.
         *
         * Runtime authority is published separately after all
         * critical runtime adapters have synchronized successfully.
         */
        MarkAuthorityUnavailable();

        if (!_configLoaded)
        {
            return {
                BootstrapStatus::InvalidDefaultState,
                "GPA startup configuration was not loaded."
            };
        }

        if (!_config.valid)
        {
            return {
                BootstrapStatus::InvalidDefaultState,
                _config.error
            };
        }

        return ProgressionBootstrap::Initialize(
            _manager,
            _store,
            _config.defaultState,
            _config.allowRollback);
    }

    bool ProgressionRuntime::SynchronizeCriticalRuntime(
        std::string& error)
    {
        /*
         * Startup synchronization happens before authority
         * publication. Pull providers must remain fail-closed until
         * every critical push participant matches the active GPA
         * state.
         */
        MarkAuthorityUnavailable();

        std::optional<ProgressionState> const active =
            _manager.GetActiveState();

        if (!active)
        {
            error =
                "Cannot synchronize critical runtime without "
                "an active GPA state.";

            return false;
        }

        if (!_criticalRuntime.Prepare(
                *active,
                error))
        {
            if (error.empty())
            {
                error =
                    "Critical runtime startup preparation failed.";
            }

            return false;
        }

        CriticalRuntimeApplyResult const applyResult =
            _criticalRuntime.ApplyPrepared();

        if (!applyResult.Succeeded())
        {
            error =
                applyResult.error.empty()
                    ? "Critical runtime startup application failed."
                    : applyResult.error;

            return false;
        }

        if (!_criticalRuntime.CommitPrepared(
                error))
        {
            if (error.empty())
            {
                error =
                    "Critical runtime startup finalization failed.";
            }

            return false;
        }

        error.clear();

        return true;
    }

    PersistentTransitionResult
    ProgressionRuntime::TransitionTo(
        ProgressionState const& target)
    {
        TransitionResult emptyTransition;
        emptyTransition.target = target;

        if (!_configLoaded)
        {
            return {
                PersistentTransitionStatus::AuthorityUnavailable,
                emptyTransition,
                "GPA startup configuration was not loaded."
            };
        }

        if (!_config.enabled)
        {
            return {
                PersistentTransitionStatus::AuthorityUnavailable,
                emptyTransition,
                "GPA runtime authority is disabled."
            };
        }

        if (!_config.valid)
        {
            return {
                PersistentTransitionStatus::AuthorityUnavailable,
                emptyTransition,
                _config.error.empty()
                    ? "GPA startup configuration is invalid."
                    : _config.error
            };
        }

        /*
         * Atomic true -> false is both:
         *
         *   - the readiness publication barrier for pull adapters;
         *   - the serialization gate preventing two runtime-level
         *     transitions from entering concurrently.
         */
        if (!TryBeginAuthorityTransition())
        {
            return {
                PersistentTransitionStatus::AuthorityUnavailable,
                emptyTransition,
                "GPA authority is unavailable or another "
                "transition is already in progress."
            };
        }

        PersistentTransitionResult result =
            _transitionCoordinator.Execute(target);

        /*
         * Re-publish authority only when both runtime coherence and
         * durable authority are known safe.
         *
         * In particular, SaveFailed remains fail-closed even when
         * runtime restoration succeeded because Save(false) does not
         * prove what happened in the durable backend.
         */
        if (result.SafeToRepublishAuthority())
            MarkAuthorityReady();

        return result;
    }
}
