#include "ProgressionRuntime.h"

#include "Config.h"
#include "Log.h"

#include <cstdint>
#include <limits>

namespace GPA
{
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

    BootstrapResult ProgressionRuntime::Bootstrap()
    {
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
}
