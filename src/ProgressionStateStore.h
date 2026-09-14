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
         * Persists the complete GPA state atomically from
         * the caller's point of view.
         *
         * The store does not decide whether Phase or
         * ContentStage are valid progression values.
         */
        virtual bool Save(
            ProgressionState const& state,
            std::string& error) = 0;
    };
}

#endif
