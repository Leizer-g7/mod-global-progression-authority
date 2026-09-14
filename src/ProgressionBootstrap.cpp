#include "ProgressionBootstrap.h"

namespace GPA
{
    BootstrapResult ProgressionBootstrap::Initialize(
        ProgressionMgr& manager,
        IProgressionStateStore& store,
        ProgressionState const& defaultState,
        bool allowRollback)
    {
        if (manager.IsInitialized())
        {
            return {
                BootstrapStatus::AlreadyInitialized,
                "ProgressionMgr is already initialized."
            };
        }

        StateLoadResult loadResult =
            store.Load();

        if (loadResult.status == StateLoadStatus::Loaded &&
            !loadResult.state)
        {
            return {
                BootstrapStatus::MalformedLoadResult,
                "State store returned Loaded without a state."
            };
        }

        if (loadResult.status == StateLoadStatus::Missing &&
            loadResult.state)
        {
            return {
                BootstrapStatus::MalformedLoadResult,
                "State store returned Missing with a state."
            };
        }

        if (loadResult.status == StateLoadStatus::Error)
        {
            std::string error =
                loadResult.error.empty()
                    ? "State store load failed."
                    : loadResult.error;

            return {
                BootstrapStatus::LoadError,
                error
            };
        }

        if (loadResult.status == StateLoadStatus::Loaded)
        {
            std::string error;

            if (!manager.ValidateState(
                    *loadResult.state,
                    error))
            {
                return {
                    BootstrapStatus::InvalidPersistedState,
                    error
                };
            }

            if (!manager.Initialize(
                    *loadResult.state,
                    allowRollback,
                    error))
            {
                return {
                    BootstrapStatus::ManagerInitializationFailed,
                    error
                };
            }

            return {
                BootstrapStatus::LoadedPersistedState,
                {}
            };
        }

        /*
         * Missing persistent state:
         *
         * Validate first, then persist, and only after
         * successful persistence publish authority in memory.
         */
        std::string error;

        if (!manager.ValidateState(
                defaultState,
                error))
        {
            return {
                BootstrapStatus::InvalidDefaultState,
                error
            };
        }

        if (!store.Save(
                defaultState,
                error))
        {
            if (error.empty())
                error = "State store save failed.";

            return {
                BootstrapStatus::SaveDefaultFailed,
                error
            };
        }

        if (!manager.Initialize(
                defaultState,
                allowRollback,
                error))
        {
            return {
                BootstrapStatus::ManagerInitializationFailed,
                error
            };
        }

        return {
            BootstrapStatus::CreatedDefaultState,
            {}
        };
    }
}
