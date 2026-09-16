#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_STATE_STORE_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_STATE_STORE_H

#include "ProgressionState.h"

#include <optional>
#include <string>

namespace GPA
{
    enum class StateLoadStatus
    {
        Loaded,
        Missing,
        Error
    };

    struct StateLoadResult
    {
        StateLoadStatus status =
            StateLoadStatus::Error;

        std::optional<ProgressionState> state;

        std::string error;

        bool Succeeded() const
        {
            return
                status == StateLoadStatus::Loaded ||
                status == StateLoadStatus::Missing;
        }
    };

    class IProgressionStateStore
    {
    public:
        virtual ~IProgressionStateStore() = default;

        /*
         * Loads the complete persisted GPA state.
         *
         * Loaded:
         *   A persistent state exists.
         *
         * Missing:
         *   No persistent state exists yet.
         *   This is not a storage error.
         *
         * Error:
         *   The persistence backend could not provide
         *   a reliable result.
         */
        virtual StateLoadResult Load() = 0;

        /*
         * Persists the complete GPA state.
         *
         * true:
         *   The backend confirmed persistence success.
         *
         * false:
         *   Persistence was not confirmed successful.
         *   The caller must not publish the target state.
         *   A false result does not by itself prove that
         *   the durable backend remained unchanged.
         *
         * The store does not decide whether Phase or
         * ContentStage are valid progression values.
         *
         * Load() and Save() may be called while GPA holds
         * its authority mutex. Store implementations must
         * not re-enter ProgressionMgr or its coordinators.
         */
        virtual bool Save(
            ProgressionState const& state,
            std::string& error) = 0;
    };
}

#endif
