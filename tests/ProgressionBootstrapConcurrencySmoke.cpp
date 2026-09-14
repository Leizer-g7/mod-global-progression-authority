#include "ProgressionBootstrap.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace
{
    class CountingStore final
        : public GPA::IProgressionStateStore
    {
    public:
        GPA::StateLoadResult Load() override
        {
            ++loadCount;

            /*
             * Keep the winning bootstrap inside the manager lock
             * long enough for the second thread to contend for it.
             */
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));

            std::lock_guard<std::mutex> lock(stateMutex);

            if (!state)
            {
                return {
                    GPA::StateLoadStatus::Missing,
                    std::nullopt,
                    {}
                };
            }

            return {
                GPA::StateLoadStatus::Loaded,
                state,
                {}
            };
        }

        bool Save(
            GPA::ProgressionState const& newState,
            std::string& error) override
        {
            ++saveCount;

            std::lock_guard<std::mutex> lock(stateMutex);

            state = newState;
            error.clear();

            return true;
        }

        std::optional<GPA::ProgressionState> GetState()
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            return state;
        }

        std::atomic<unsigned> loadCount{0};
        std::atomic<unsigned> saveCount{0};

    private:
        std::mutex stateMutex;
        std::optional<GPA::ProgressionState> state;
    };

    bool IsCreated(
        GPA::BootstrapResult const& result)
    {
        return
            result.status ==
                GPA::BootstrapStatus::CreatedDefaultState;
    }

    bool IsAlreadyInitialized(
        GPA::BootstrapResult const& result)
    {
        return
            result.status ==
                GPA::BootstrapStatus::AlreadyInitialized;
    }
}

int main()
{
    GPA::ProgressionMgr manager;
    CountingStore store;

    std::atomic<bool> start{false};

    GPA::BootstrapResult resultA;
    GPA::BootstrapResult resultB;

    std::thread threadA([&]
    {
        while (!start.load())
            std::this_thread::yield();

        resultA =
            GPA::ProgressionBootstrap::Initialize(
                manager,
                store,
                {20, 0},
                false);
    });

    std::thread threadB([&]
    {
        while (!start.load())
            std::this_thread::yield();

        resultB =
            GPA::ProgressionBootstrap::Initialize(
                manager,
                store,
                {30, 0},
                false);
    });

    start.store(true);

    threadA.join();
    threadB.join();

    bool const aWon =
        IsCreated(resultA) &&
        IsAlreadyInitialized(resultB);

    bool const bWon =
        IsCreated(resultB) &&
        IsAlreadyInitialized(resultA);

    if (!aWon && !bWon)
        return 1;

    if (store.loadCount.load() != 1)
        return 2;

    if (store.saveCount.load() != 1)
        return 3;

    auto active =
        manager.GetActiveState();

    auto persisted =
        store.GetState();

    if (!active || !persisted)
        return 4;

    if (*active != *persisted)
        return 5;

    bool const validWinner =
        *active == GPA::ProgressionState{20, 0} ||
        *active == GPA::ProgressionState{30, 0};

    if (!validWinner)
        return 6;

    std::cout
        << "CONCURRENT_BOOTSTRAP_SINGLE_WINNER=OK\n"
        << "CONCURRENT_BOOTSTRAP_LOAD_COUNT=1\n"
        << "CONCURRENT_BOOTSTRAP_SAVE_COUNT=1\n"
        << "CONCURRENT_BOOTSTRAP_STATE_CONSISTENCY=OK\n"
        << "CONCURRENT_BOOTSTRAP_SECOND_STORE_ACCESS=NONE\n";

    return 0;
}
