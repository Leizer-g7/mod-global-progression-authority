#include "MemoryProgressionStateStore.h"
#include "ProgressionBootstrap.h"

#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace
{
    bool StateEquals(
        GPA::ProgressionMgr const& manager,
        std::uint8_t phase,
        std::uint8_t stage)
    {
        auto state =
            manager.GetActiveState();

        return state &&
            state->phase == phase &&
            state->contentStage == stage;
    }

    class FixedLoadStore final
        : public GPA::IProgressionStateStore
    {
    public:
        explicit FixedLoadStore(
            GPA::StateLoadResult result)
            : _result(std::move(result))
        {
        }

        GPA::StateLoadResult Load() override
        {
            return _result;
        }

        bool Save(
            GPA::ProgressionState const&,
            std::string& error) override
        {
            error = "Unexpected Save call.";
            return false;
        }

    private:
        GPA::StateLoadResult _result;
    };
}

int main()
{
    using GPA::BootstrapStatus;
    using GPA::ProgressionBootstrap;
    using GPA::ProgressionMgr;
    using GPA::StateLoadStatus;
    using GPA::Test::MemoryProgressionStateStore;

    /*
     * Loaded valid persisted state.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({70, 8});

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                true);

        if (result.status !=
                BootstrapStatus::LoadedPersistedState ||
            !result.Succeeded() ||
            !StateEquals(manager, 70, 8) ||
            !manager.IsRollbackAllowed() ||
            store.GetSaveCount() != 0)
        {
            return 1;
        }
    }

    /*
     * Missing state creates and persists default.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store;

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);

        if (result.status !=
                BootstrapStatus::CreatedDefaultState ||
            !result.Succeeded() ||
            !StateEquals(manager, 20, 0) ||
            store.GetSaveCount() != 1 ||
            !store.GetStoredState() ||
            *store.GetStoredState() !=
                GPA::ProgressionState{20, 0})
        {
            return 2;
        }
    }

    /*
     * Load error must not create authority.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store;

        store.SetLoadError("simulated load failure");

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);

        if (result.status != BootstrapStatus::LoadError ||
            result.error.empty() ||
            manager.IsInitialized() ||
            store.GetSaveCount() != 0)
        {
            return 3;
        }
    }

    /*
     * Invalid persisted state must not be repaired.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store({20, 1});

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);

        if (result.status !=
                BootstrapStatus::InvalidPersistedState ||
            result.error.empty() ||
            manager.IsInitialized() ||
            store.GetSaveCount() != 0)
        {
            return 4;
        }
    }

    /*
     * Invalid default must not be persisted.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store;

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 1},
                false);

        if (result.status !=
                BootstrapStatus::InvalidDefaultState ||
            result.error.empty() ||
            manager.IsInitialized() ||
            store.GetSaveCount() != 0 ||
            store.GetStoredState())
        {
            return 5;
        }
    }

    /*
     * Save failure must leave manager uninitialized.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store;

        store.SetSaveError("simulated save failure");

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);

        if (result.status !=
                BootstrapStatus::SaveDefaultFailed ||
            result.error.empty() ||
            manager.IsInitialized() ||
            store.GetSaveCount() != 0 ||
            store.GetStoredState())
        {
            return 6;
        }
    }

    /*
     * Already initialized manager must win immediately.
     */
    {
        ProgressionMgr manager;
        MemoryProgressionStateStore store;
        std::string error;

        if (!manager.Initialize({60, 0}, false, error))
            return 7;

        store.SetLoadError(
            "This must never become the bootstrap result.");

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);

        if (result.status !=
                BootstrapStatus::AlreadyInitialized ||
            !StateEquals(manager, 60, 0))
        {
            return 8;
        }
    }

    /*
     * Malformed: Loaded without state.
     */
    {
        ProgressionMgr manager;

        FixedLoadStore store({
            StateLoadStatus::Loaded,
            std::nullopt,
            {}
        });

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);

        if (result.status !=
                BootstrapStatus::MalformedLoadResult ||
            manager.IsInitialized())
        {
            return 9;
        }
    }

    /*
     * Malformed: Missing with state.
     */
    {
        ProgressionMgr manager;

        FixedLoadStore store({
            StateLoadStatus::Missing,
            GPA::ProgressionState{20, 0},
            {}
        });

        auto result =
            ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);

        if (result.status !=
                BootstrapStatus::MalformedLoadResult ||
            manager.IsInitialized())
        {
            return 10;
        }
    }

    std::cout
        << "BOOTSTRAP_LOADED_STATE=OK\n"
        << "BOOTSTRAP_MISSING_DEFAULT=OK\n"
        << "BOOTSTRAP_LOAD_ERROR=OK\n"
        << "BOOTSTRAP_INVALID_PERSISTED=OK\n"
        << "BOOTSTRAP_INVALID_DEFAULT=OK\n"
        << "BOOTSTRAP_SAVE_FAILURE=OK\n"
        << "BOOTSTRAP_ALREADY_INITIALIZED=OK\n"
        << "BOOTSTRAP_MALFORMED_RESULTS=OK\n"
        << "BOOTSTRAP_NO_SILENT_REPAIR=OK\n";

    return 0;
}
