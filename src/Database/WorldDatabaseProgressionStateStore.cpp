#include "WorldDatabaseProgressionStateStore.h"

#include "DatabaseEnv.h"
#include "QueryResult.h"

#include <cstdint>
#include <limits>

namespace GPA
{
    StateLoadResult
    WorldDatabaseProgressionStateStore::Load()
    {
        /*
         * The aggregate query deliberately always returns one row
         * when SQL execution succeeds.
         *
         * This lets GPA distinguish:
         *
         *   QueryResult == nullptr -> storage/query error
         *   row_count == 0         -> canonical row missing
         *   row_count == 1         -> canonical row loaded
         *
         * A normal SELECT of id=1 cannot make that distinction
         * through AzerothCore's Query() API because zero rows and
         * query failure both surface as a null QueryResult.
         */
        QueryResult result =
            WorldDatabase.Query(
                "SELECT "
                "COUNT(*) AS `row_count`, "
                "COALESCE(MAX(`active_phase`), 0) AS `active_phase`, "
                "COALESCE(MAX(`active_content_stage`), 0) "
                "AS `active_content_stage` "
                "FROM `phase_progression_state` "
                "WHERE `id` = 1");

        if (!result)
        {
            return {
                StateLoadStatus::Error,
                std::nullopt,
                "Unable to query the persisted GPA state."
            };
        }

        Field* fields =
            result->Fetch();

        if (!fields)
        {
            return {
                StateLoadStatus::Error,
                std::nullopt,
                "GPA state query returned no fields."
            };
        }

        std::uint64_t const rowCount =
            fields[0].Get<std::uint64_t>();

        if (rowCount == 0)
        {
            return {
                StateLoadStatus::Missing,
                std::nullopt,
                {}
            };
        }

        if (rowCount != 1)
        {
            return {
                StateLoadStatus::Error,
                std::nullopt,
                "GPA canonical state query returned an unexpected row count."
            };
        }

        std::uint64_t const phase =
            fields[1].Get<std::uint64_t>();

        std::uint64_t const contentStage =
            fields[2].Get<std::uint64_t>();

        constexpr std::uint64_t maxStateValue =
            std::numeric_limits<std::uint8_t>::max();

        if (phase > maxStateValue ||
            contentStage > maxStateValue)
        {
            return {
                StateLoadStatus::Error,
                std::nullopt,
                "Persisted GPA state exceeds the supported storage range."
            };
        }

        return {
            StateLoadStatus::Loaded,
            ProgressionState{
                static_cast<std::uint8_t>(phase),
                static_cast<std::uint8_t>(contentStage)
            },
            {}
        };
    }

    bool WorldDatabaseProgressionStateStore::Save(
        ProgressionState const& state,
        std::string& error)
    {
        /*
         * Persist the complete GPA state in one idempotent statement.
         *
         * A false result means the synchronous write was not confirmed
         * successful. The caller must not publish the target state to
         * memory in that case.
         */
        std::uint32_t const phase =
            static_cast<std::uint32_t>(state.phase);

        std::uint32_t const contentStage =
            static_cast<std::uint32_t>(state.contentStage);

        bool const success =
            WorldDatabase.DirectExecuteWithResult(
                "INSERT INTO `phase_progression_state` "
                "(`id`, `active_phase`, `active_content_stage`) "
                "VALUES (1, {}, {}) "
                "ON DUPLICATE KEY UPDATE "
                "`active_phase` = {}, "
                "`active_content_stage` = {}",
                phase,
                contentStage,
                phase,
                contentStage);

        if (!success)
        {
            error =
                "WorldDatabase did not confirm persistence "
                "of the GPA progression state.";

            return false;
        }

        error.clear();
        return true;
    }
}
