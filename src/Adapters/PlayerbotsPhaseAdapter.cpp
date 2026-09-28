#include "PlayerbotsPhaseAdapter.h"

#if !__has_include("PlayerbotAIConfig.h") || \
    !__has_include("RandomBotLevelMgr.h")

namespace GPA
{
    ICriticalRuntimeParticipant*
    GetPlayerbotsPhaseAdapter()
    {
        /*
         * mod-playerbots is optional.
         *
         * AzerothCore currently defines MOD_PLAYERBOTS from source-tree
         * discovery even when MODULE_MOD-PLAYERBOTS=disabled, so that
         * preprocessor symbol cannot be used as an enabled-build guard.
         *
         * The module include paths, however, are only exported when
         * Playerbots is actually part of the build. In a build without
         * those headers, expose the same GPA boundary as an unavailable
         * optional participant.
         */
        return nullptr;
    }
}

#else

#include "AdapterPolicies.h"
#include "PhaseCatalog.h"
#include "PlayerbotsPolicyCatalog.h"
#include "ProgressionState.h"
#include "Runtime/CriticalRuntimeCoordinator.h"

#include "Config.h"
#include "Log.h"
#include "PlayerbotAIConfig.h"
#include "RandomBotLevelMgr.h"

#include <cctype>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    std::string Trim(std::string value)
    {
        while (!value.empty() &&
               std::isspace(
                   static_cast<unsigned char>(
                       value.front())))
        {
            value.erase(value.begin());
        }

        while (!value.empty() &&
               std::isspace(
                   static_cast<unsigned char>(
                       value.back())))
        {
            value.pop_back();
        }

        return value;
    }

    bool ParseUInt(
        std::string const& text,
        std::uint32_t& result)
    {
        if (text.empty())
            return false;

        try
        {
            std::size_t processed = 0;

            unsigned long const parsed =
                std::stoul(
                    text,
                    &processed);

            if (processed != text.size())
                return false;

            if (parsed >
                std::numeric_limits<
                    std::uint32_t>::max())
            {
                return false;
            }

            result =
                static_cast<std::uint32_t>(
                    parsed);

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool ParseDistribution(
        std::string const& text,
        std::uint8_t maxLevel,
        std::vector<LevelBracketConfig>& result,
        std::string& error)
    {
        result.clear();

        if (!maxLevel)
        {
            error =
                "Playerbots max level cannot be zero.";

            return false;
        }

        std::stringstream stream(text);
        std::string token;

        std::uint32_t percentageTotal = 0;
        std::uint32_t previousUpper = 0;

        while (std::getline(
            stream,
            token,
            ','))
        {
            token = Trim(token);

            if (token.empty())
                continue;

            std::size_t const colon =
                token.find(':');

            if (colon == std::string::npos)
            {
                error =
                    "Invalid Playerbots distribution bracket '" +
                    token +
                    "': missing ':'.";

                return false;
            }

            std::string const rangeText =
                Trim(
                    token.substr(
                        0,
                        colon));

            std::string const pctText =
                Trim(
                    token.substr(
                        colon + 1));

            std::uint32_t lower = 0;
            std::uint32_t upper = 0;
            std::uint32_t pct = 0;

            std::size_t const dash =
                rangeText.find('-');

            if (dash == std::string::npos)
            {
                if (!ParseUInt(
                    rangeText,
                    lower))
                {
                    error =
                        "Invalid level in Playerbots bracket '" +
                        token +
                        "'.";

                    return false;
                }

                upper = lower;
            }
            else
            {
                std::string const lowerText =
                    Trim(
                        rangeText.substr(
                            0,
                            dash));

                std::string const upperText =
                    Trim(
                        rangeText.substr(
                            dash + 1));

                if (!ParseUInt(
                        lowerText,
                        lower) ||
                    !ParseUInt(
                        upperText,
                        upper))
                {
                    error =
                        "Invalid range in Playerbots bracket '" +
                        token +
                        "'.";

                    return false;
                }
            }

            if (!ParseUInt(
                pctText,
                pct))
            {
                error =
                    "Invalid percentage in Playerbots bracket '" +
                    token +
                    "'.";

                return false;
            }

            if (!lower ||
                !upper ||
                lower > upper)
            {
                error =
                    "Invalid bounds in Playerbots bracket '" +
                    token +
                    "'.";

                return false;
            }

            if (upper > maxLevel)
            {
                error =
                    "Playerbots bracket exceeds phase max level in '" +
                    token +
                    "'.";

                return false;
            }

            if (lower >
                    std::numeric_limits<
                        std::uint8_t>::max() ||
                upper >
                    std::numeric_limits<
                        std::uint8_t>::max())
            {
                error =
                    "Playerbots bracket level exceeds uint8 range.";

                return false;
            }

            if (pct > 100)
            {
                error =
                    "Playerbots bracket percentage exceeds 100 in '" +
                    token +
                    "'.";

                return false;
            }

            if (result.empty())
            {
                if (lower != 1)
                {
                    error =
                        "First Playerbots bracket must start at level 1.";

                    return false;
                }
            }
            else if (lower !=
                     previousUpper + 1)
            {
                error =
                    "Playerbots brackets must be contiguous near '" +
                    token +
                    "'.";

                return false;
            }

            LevelBracketConfig bracket;

            bracket.lower =
                static_cast<uint8>(lower);

            bracket.upper =
                static_cast<uint8>(upper);

            bracket.pct =
                static_cast<uint8>(pct);

            result.push_back(bracket);

            previousUpper = upper;
            percentageTotal += pct;
        }

        if (result.empty())
        {
            error =
                "Playerbots distribution cannot be empty.";

            return false;
        }

        if (previousUpper != maxLevel)
        {
            error =
                "Final Playerbots bracket must end at phase max level.";

            return false;
        }

        if (percentageTotal != 100)
        {
            error =
                "Playerbots distribution percentages must total exactly 100.";

            return false;
        }

        error.clear();
        return true;
    }

    struct PlayerbotsConfigSnapshot
    {
        std::uint32_t randomBotMaxLevel = 0;

        int shattrathWeight = 0;
        int dalaranWeight = 0;

        bool bracketsEnabled = false;

        std::uint32_t bracketsCheckFrequency = 0;

        std::uint32_t flaggedCheckFrequency = 0;

        std::uint32_t flaggedProcessLimit = 0;

        bool ignoreGuildWithRealPlayers = false;
        bool ignoreArenaTeamBots = false;
        bool ignoreFriendListed = false;

        std::uint8_t numRanges = 0;

        std::vector<LevelBracketConfig>
            allianceRanges;

        std::vector<LevelBracketConfig>
            hordeRanges;

        bool dynamicDistribution = false;
        bool syncFactions = false;

        std::int32_t randomGearScoreLimit = 0;
        std::int32_t autoGearScoreLimit = 0;

        bool disableDeathKnightLogin = false;

        std::uint32_t smartScaleMaxLevel = 0;
    };

    PlayerbotsConfigSnapshot
    CapturePlayerbotsConfig()
    {
        PlayerbotsConfigSnapshot snapshot;

        snapshot.randomBotMaxLevel =
            sPlayerbotAIConfig.randomBotMaxLevel;

        snapshot.shattrathWeight =
            sPlayerbotAIConfig.weightTeleToShattrathCity;

        snapshot.dalaranWeight =
            sPlayerbotAIConfig.weightTeleToDalaran;

        snapshot.bracketsEnabled =
            sPlayerbotAIConfig.levelBracketsEnabled;

        snapshot.bracketsCheckFrequency =
            sPlayerbotAIConfig.levelBracketsCheckFrequency;

        snapshot.flaggedCheckFrequency =
            sPlayerbotAIConfig.levelBracketsFlaggedCheckFrequency;

        snapshot.flaggedProcessLimit =
            sPlayerbotAIConfig.levelBracketsFlaggedProcessLimit;

        snapshot.ignoreGuildWithRealPlayers =
            sPlayerbotAIConfig
                .levelBracketsIgnoreGuildWithRealPlayers;

        snapshot.ignoreArenaTeamBots =
            sPlayerbotAIConfig
                .levelBracketsIgnoreArenaTeamBots;

        snapshot.ignoreFriendListed =
            sPlayerbotAIConfig
                .levelBracketsIgnoreFriendListed;

        snapshot.numRanges =
            sPlayerbotAIConfig.levelBracketsNumRanges;

        snapshot.allianceRanges =
            sPlayerbotAIConfig.levelBracketsAlliance;

        snapshot.hordeRanges =
            sPlayerbotAIConfig.levelBracketsHorde;

        snapshot.dynamicDistribution =
            sPlayerbotAIConfig
                .levelBracketsDynamicDistribution;

        snapshot.syncFactions =
            sPlayerbotAIConfig
                .levelBracketsSyncFactions;

        snapshot.randomGearScoreLimit =
            sPlayerbotAIConfig.randomGearScoreLimit;

        snapshot.autoGearScoreLimit =
            sPlayerbotAIConfig.autoGearScoreLimit;

        snapshot.disableDeathKnightLogin =
            sPlayerbotAIConfig.disableDeathKnightLogin;

        snapshot.smartScaleMaxLevel =
            sPlayerbotAIConfig
                .botActiveAloneSmartScaleWhenMaxLevel;

        return snapshot;
    }

    void RestorePlayerbotsConfig(
        PlayerbotsConfigSnapshot const& snapshot)
    {
        sPlayerbotAIConfig.randomBotMaxLevel =
            snapshot.randomBotMaxLevel;

        sPlayerbotAIConfig.weightTeleToShattrathCity =
            snapshot.shattrathWeight;

        sPlayerbotAIConfig.weightTeleToDalaran =
            snapshot.dalaranWeight;

        sPlayerbotAIConfig.levelBracketsEnabled =
            snapshot.bracketsEnabled;

        sPlayerbotAIConfig.levelBracketsCheckFrequency =
            snapshot.bracketsCheckFrequency;

        sPlayerbotAIConfig
            .levelBracketsFlaggedCheckFrequency =
            snapshot.flaggedCheckFrequency;

        sPlayerbotAIConfig
            .levelBracketsFlaggedProcessLimit =
            snapshot.flaggedProcessLimit;

        sPlayerbotAIConfig
            .levelBracketsIgnoreGuildWithRealPlayers =
            snapshot.ignoreGuildWithRealPlayers;

        sPlayerbotAIConfig
            .levelBracketsIgnoreArenaTeamBots =
            snapshot.ignoreArenaTeamBots;

        sPlayerbotAIConfig
            .levelBracketsIgnoreFriendListed =
            snapshot.ignoreFriendListed;

        sPlayerbotAIConfig.levelBracketsNumRanges =
            snapshot.numRanges;

        sPlayerbotAIConfig.levelBracketsAlliance =
            snapshot.allianceRanges;

        sPlayerbotAIConfig.levelBracketsHorde =
            snapshot.hordeRanges;

        sPlayerbotAIConfig
            .levelBracketsDynamicDistribution =
            snapshot.dynamicDistribution;

        sPlayerbotAIConfig.levelBracketsSyncFactions =
            snapshot.syncFactions;

        sPlayerbotAIConfig.randomGearScoreLimit =
            snapshot.randomGearScoreLimit;

        sPlayerbotAIConfig.autoGearScoreLimit =
            snapshot.autoGearScoreLimit;

        sPlayerbotAIConfig.disableDeathKnightLogin =
            snapshot.disableDeathKnightLogin;

        sPlayerbotAIConfig
            .botActiveAloneSmartScaleWhenMaxLevel =
            snapshot.smartScaleMaxLevel;
    }

    struct PreparedPlayerbotsTarget
    {
        std::uint8_t phase = 0;
        std::uint8_t maxLevel = 0;

        std::uint32_t gearMaxItemLevel = 0;

        bool outlandEnabled = false;
        bool northrendEnabled = false;
        bool deathKnightEnabled = false;

        int configuredShattrathWeight = 0;
        int configuredDalaranWeight = 0;

        std::vector<LevelBracketConfig> ranges;
    };
}

namespace GPA
{
    class PlayerbotsPhaseAdapter final
        : public ICriticalRuntimeParticipant
    {
    public:
        std::string_view Name() const override
        {
            return "PlayerbotsPhaseAdapter";
        }

        bool Prepare(
            ProgressionState const& target,
            std::string& error) override
        {
            if (_preparedTarget ||
                _configSnapshot ||
                _runtimeSnapshot)
            {
                error =
                    "Playerbots phase adapter already has a prepared transaction.";

                return false;
            }

            PhaseDefinition const* const definition =
                _phaseCatalog.Find(
                    target.phase);

            if (!definition)
            {
                error =
                    "Playerbots phase adapter cannot find target phase definition.";

                return false;
            }

            PlayerbotsPhasePolicy const* const policy =
                _policyCatalog.Find(
                    target.phase);

            if (!policy)
            {
                error =
                    "Playerbots phase adapter cannot find target policy.";

                return false;
            }

            if (definition->playerMaxLevel !=
                policy->maxLevel)
            {
                error =
                    "Playerbots max level does not match phase player cap.";

                return false;
            }

            if (policy->gearMaxItemLevel >
                static_cast<std::uint32_t>(
                    std::numeric_limits<
                        std::int32_t>::max()))
            {
                error =
                    "Playerbots gear item level exceeds runtime field range.";

                return false;
            }

            PreparedPlayerbotsTarget prepared;

            prepared.phase =
                target.phase;

            prepared.maxLevel =
                policy->maxLevel;

            prepared.gearMaxItemLevel =
                policy->gearMaxItemLevel;

            prepared.outlandEnabled =
                definition->world.outlandEnabled;

            prepared.northrendEnabled =
                definition->world.northrendEnabled;

            prepared.deathKnightEnabled =
                definition->deathKnightEnabled;

            if (!ParseDistribution(
                    policy->distribution,
                    policy->maxLevel,
                    prepared.ranges,
                    error))
            {
                return false;
            }

            if (prepared.ranges.size() >
                std::numeric_limits<
                    std::uint8_t>::max())
            {
                error =
                    "Playerbots distribution contains too many ranges.";

                return false;
            }

            prepared.configuredShattrathWeight =
                sConfigMgr->GetOption<int>(
                    "AiPlayerbot.TeleToShattrathCityWeight",
                    1);

            prepared.configuredDalaranWeight =
                sConfigMgr->GetOption<int>(
                    "AiPlayerbot.TeleToDalaranWeight",
                    1);

            try
            {
                PlayerbotsConfigSnapshot configSnapshot =
                    CapturePlayerbotsConfig();

                RandomBotLevelRuntimeConfigSnapshot
                    runtimeSnapshot =
                        RandomBotLevelMgr::instance()
                            .CaptureRuntimeConfigSnapshot();

                _preparedTarget =
                    std::move(prepared);

                _configSnapshot =
                    std::move(configSnapshot);

                _runtimeSnapshot =
                    std::move(runtimeSnapshot);
            }
            catch (std::exception const& ex)
            {
                CancelPrepared();

                error =
                    "Playerbots runtime snapshot failed: ";

                error += ex.what();

                return false;
            }
            catch (...)
            {
                CancelPrepared();

                error =
                    "Playerbots runtime snapshot failed with unknown exception.";

                return false;
            }

            error.clear();
            return true;
        }

        bool ApplyPrepared(
            std::string& error) override
        {
            if (!_preparedTarget ||
                !_configSnapshot ||
                !_runtimeSnapshot)
            {
                error =
                    "Playerbots phase adapter has no prepared transaction.";

                return false;
            }

            PreparedPlayerbotsTarget const& target =
                *_preparedTarget;

            try
            {
                sPlayerbotAIConfig.randomBotMaxLevel =
                    target.maxLevel;

                sPlayerbotAIConfig
                    .weightTeleToShattrathCity =
                    target.outlandEnabled
                        ? target.configuredShattrathWeight
                        : 0;

                sPlayerbotAIConfig
                    .weightTeleToDalaran =
                    target.northrendEnabled
                        ? target.configuredDalaranWeight
                        : 0;

                sPlayerbotAIConfig.levelBracketsEnabled =
                    true;

                sPlayerbotAIConfig
                    .levelBracketsIgnoreGuildWithRealPlayers =
                    false;

                sPlayerbotAIConfig
                    .levelBracketsIgnoreArenaTeamBots =
                    false;

                sPlayerbotAIConfig
                    .levelBracketsIgnoreFriendListed =
                    false;

                sPlayerbotAIConfig
                    .levelBracketsCheckFrequency =
                    30;

                sPlayerbotAIConfig
                    .levelBracketsFlaggedCheckFrequency =
                    10;

                sPlayerbotAIConfig
                    .levelBracketsFlaggedProcessLimit =
                    10;

                sPlayerbotAIConfig
                    .levelBracketsDynamicDistribution =
                    false;

                sPlayerbotAIConfig
                    .levelBracketsSyncFactions =
                    false;

                sPlayerbotAIConfig
                    .levelBracketsNumRanges =
                    static_cast<uint8>(
                        target.ranges.size());

                sPlayerbotAIConfig.levelBracketsAlliance =
                    target.ranges;

                sPlayerbotAIConfig.levelBracketsHorde =
                    target.ranges;

                sPlayerbotAIConfig.randomGearScoreLimit =
                    static_cast<int32>(
                        target.gearMaxItemLevel);

                sPlayerbotAIConfig.autoGearScoreLimit =
                    static_cast<int32>(
                        target.gearMaxItemLevel);

                sPlayerbotAIConfig.disableDeathKnightLogin =
                    !target.deathKnightEnabled;

                sPlayerbotAIConfig
                    .botActiveAloneSmartScaleWhenMaxLevel =
                    target.maxLevel;

                /*
                 * RandomBotLevelMgr owns runtime working copies.
                 * Rebuild those copies from the GPA-controlled
                 * PlayerbotAIConfig target.
                 */
                RandomBotLevelMgr::instance()
                    .LoadConfig();
            }
            catch (std::exception const& ex)
            {
                error =
                    "Playerbots runtime apply failed: ";

                error += ex.what();

                return false;
            }
            catch (...)
            {
                error =
                    "Playerbots runtime apply failed with unknown exception.";

                return false;
            }

            LOG_INFO(
                "module",
                "GPA: Playerbots runtime applied. "
                "Phase={}, MaxLevel={}, Ranges={}, "
                "GearMaxILvl={}, DK={}, "
                "ShattrathWeight={}, DalaranWeight={}.",
                static_cast<unsigned>(
                    target.phase),
                static_cast<unsigned>(
                    target.maxLevel),
                target.ranges.size(),
                target.gearMaxItemLevel,
                target.deathKnightEnabled
                    ? "ENABLED"
                    : "DISABLED",
                sPlayerbotAIConfig
                    .weightTeleToShattrathCity,
                sPlayerbotAIConfig
                    .weightTeleToDalaran);

            error.clear();
            return true;
        }

        bool RestorePrepared(
            std::string& error) override
        {
            if (!_configSnapshot ||
                !_runtimeSnapshot)
            {
                error =
                    "Playerbots phase adapter has no runtime snapshot to restore.";

                return false;
            }

            try
            {
                /*
                 * Restore public Playerbot configuration first.
                 */
                RestorePlayerbotsConfig(
                    *_configSnapshot);

                /*
                 * Restore the exact RandomBotLevelMgr working
                 * copies captured by Prepare().
                 *
                 * Deliberately do NOT call LoadConfig() here:
                 * LoadConfig() would reconstruct and normalize
                 * working ranges rather than restoring them.
                 */
                RandomBotLevelMgr::instance()
                    .RestoreRuntimeConfigSnapshot(
                        *_runtimeSnapshot);
            }
            catch (std::exception const& ex)
            {
                error =
                    "Playerbots runtime restore failed: ";

                error += ex.what();

                return false;
            }
            catch (...)
            {
                error =
                    "Playerbots runtime restore failed with unknown exception.";

                return false;
            }

            LOG_INFO(
                "module",
                "GPA: Playerbots runtime restored from transaction snapshot.");

            error.clear();
            return true;
        }

        void CommitPrepared() override
        {
            ResetPrepared();
        }

        void CancelPrepared() override
        {
            ResetPrepared();
        }

    private:
        void ResetPrepared()
        {
            _preparedTarget.reset();
            _configSnapshot.reset();
            _runtimeSnapshot.reset();
        }

        PhaseCatalog _phaseCatalog;
        PlayerbotsPolicyCatalog _policyCatalog;

        std::optional<PreparedPlayerbotsTarget>
            _preparedTarget;

        std::optional<PlayerbotsConfigSnapshot>
            _configSnapshot;

        std::optional<
            RandomBotLevelRuntimeConfigSnapshot>
            _runtimeSnapshot;
    };

    ICriticalRuntimeParticipant*
    GetPlayerbotsPhaseAdapter()
    {
        static PlayerbotsPhaseAdapter adapter;
        return &adapter;
    }
}

#endif // Playerbots headers available
