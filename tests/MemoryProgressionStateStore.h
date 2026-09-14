#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_MEMORY_PROGRESSION_STATE_STORE_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_MEMORY_PROGRESSION_STATE_STORE_H

#include "ProgressionStateStore.h"

#include <optional>
#include <string>
#include <utility>

namespace GPA::Test
{
    class MemoryProgressionStateStore final
        : public IProgressionStateStore
    {
    public:
        MemoryProgressionStateStore() = default;

        explicit MemoryProgressionStateStore(
            ProgressionState const& initialState)
            : _state(initialState)
        {
        }

        StateLoadResult Load() override
        {
            if (!_loadError.empty())
            {
                return {
                    StateLoadStatus::Error,
                    std::nullopt,
                    _loadError
                };
            }

            if (!_state)
            {
                return {
                    StateLoadStatus::Missing,
                    std::nullopt,
                    {}
                };
            }

            return {
                StateLoadStatus::Loaded,
                _state,
                {}
            };
        }

        bool Save(
            ProgressionState const& state,
            std::string& error) override
        {
            if (!_saveError.empty())
            {
                error = _saveError;
                return false;
            }

            _state = state;
            ++_saveCount;

            error.clear();
            return true;
        }

        void SetLoadError(std::string error)
        {
            _loadError = std::move(error);
        }

        void SetSaveError(std::string error)
        {
            _saveError = std::move(error);
        }

        std::optional<ProgressionState> const&
        GetStoredState() const
        {
            return _state;
        }

        unsigned GetSaveCount() const
        {
            return _saveCount;
        }

    private:
        std::optional<ProgressionState> _state;

        std::string _loadError;
        std::string _saveError;

        unsigned _saveCount = 0;
    };
}

#endif
