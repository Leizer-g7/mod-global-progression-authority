#include "ProgressionMgr.h"

#include <iostream>
#include <optional>
#include <string>

namespace
{
    bool Expect(
        bool condition,
        char const* message)
    {
        if (condition)
            return true;

        std::cerr
            << "FAIL: "
            << message
            << '\n';

        return false;
    }

    bool StateEquals(
        std::optional<GPA::ProgressionState> const& state,
        std::uint8_t phase,
        std::uint8_t stage)
    {
        return
            state &&
            state->phase == phase &&
            state->contentStage == stage;
    }
}

int main()
{
    using GPA::ProgressionMgr;
    using GPA::TransitionDirection;
    using GPA::TransitionStatus;

    /*
     * Uninitialized manager.
     */
    ProgressionMgr manager;

    if (!Expect(
            !manager.IsInitialized(),
            "Manager must start uninitialized.") ||
        !Expect(
            !manager.GetActiveState().has_value(),
            "Uninitialized manager exposed active state.") ||
        !Expect(
            manager.GetActiveDefinition() == nullptr,
            "Uninitialized manager exposed active definition."))
    {
        return 1;
    }

    auto beforeInit =
        manager.EvaluateTransition({20, 0});

    if (!Expect(
            beforeInit.status ==
                TransitionStatus::NotInitialized,
            "Evaluate before initialization must fail.") ||
        !Expect(
            !beforeInit.previous.has_value(),
            "Uninitialized evaluation invented previous state."))
    {
        return 2;
    }

    /*
     * Invalid initialization.
     */
    std::string error;

    if (!Expect(
            !manager.Initialize(
                {20, 1},
                false,
                error),
            "Invalid initial state was accepted.") ||
        !Expect(
            !manager.IsInitialized(),
            "Invalid initialization changed authority."))
    {
        return 3;
    }

    /*
     * Valid initialization.
     */
    error.clear();

    if (!Expect(
            manager.Initialize(
                {60, 1},
                false,
                error),
            "Valid initialization failed.") ||
        !Expect(
            error.empty(),
            "Successful initialization returned an error.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "Initial state mismatch.") ||
        !Expect(
            !manager.IsRollbackAllowed(),
            "Rollback unexpectedly enabled."))
    {
        return 4;
    }

    /*
     * Second initialization is forbidden.
     */
    error.clear();

    if (!Expect(
            !manager.Initialize(
                {70, 8},
                false,
                error),
            "Second initialization was accepted.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "Second initialization changed authority."))
    {
        return 5;
    }

    /*
     * NoChange evaluation is read-only.
     */
    auto noChange =
        manager.EvaluateTransition({60, 1});

    if (!Expect(
            noChange.status ==
                TransitionStatus::NoChange,
            "NoChange evaluation returned wrong status.") ||
        !Expect(
            noChange.direction ==
                TransitionDirection::NoChange,
            "NoChange evaluation returned wrong direction.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "NoChange evaluation modified authority."))
    {
        return 6;
    }

    /*
     * Forward evaluation is allowed and read-only.
     */
    auto forward =
        manager.EvaluateTransition({60, 2});

    if (!Expect(
            forward.status ==
                TransitionStatus::Allowed,
            "Forward evaluation was not allowed.") ||
        !Expect(
            forward.direction ==
                TransitionDirection::Forward,
            "Forward evaluation returned wrong direction.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "Forward evaluation modified authority."))
    {
        return 7;
    }

    /*
     * Invalid target.
     */
    auto invalid =
        manager.EvaluateTransition({60, 9});

    if (!Expect(
            invalid.status ==
                TransitionStatus::InvalidTarget,
            "Invalid target returned wrong status.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "Invalid evaluation modified authority."))
    {
        return 8;
    }

    /*
     * Rollback blocked by policy.
     */
    auto blockedRollback =
        manager.EvaluateTransition({60, 0});

    if (!Expect(
            blockedRollback.status ==
                TransitionStatus::RollbackNotAllowed,
            "Blocked rollback returned wrong status.") ||
        !Expect(
            blockedRollback.direction ==
                TransitionDirection::Rollback,
            "Blocked rollback returned wrong direction."))
    {
        return 9;
    }

    /*
     * Shared boundary is forward.
     */
    auto boundary =
        manager.EvaluateTransition({70, 8});

    if (!Expect(
            boundary.status ==
                TransitionStatus::Allowed,
            "60 -> 70 boundary was not allowed.") ||
        !Expect(
            boundary.direction ==
                TransitionDirection::Forward,
            "60 -> 70 boundary direction mismatch."))
    {
        return 10;
    }

    /*
     * Rollback-enabled manager evaluates rollback as Allowed.
     */
    ProgressionMgr rollbackManager;

    error.clear();

    if (!Expect(
            rollbackManager.Initialize(
                {80, 13},
                true,
                error),
            "Rollback manager initialization failed."))
    {
        return 11;
    }

    auto rollback =
        rollbackManager.EvaluateTransition({70, 13});

    if (!Expect(
            rollback.status ==
                TransitionStatus::Allowed,
            "Allowed rollback evaluation was rejected.") ||
        !Expect(
            rollback.direction ==
                TransitionDirection::Rollback,
            "Allowed rollback direction mismatch.") ||
        !Expect(
            StateEquals(
                rollbackManager.GetActiveState(),
                80,
                13),
            "Rollback evaluation modified authority."))
    {
        return 12;
    }

    std::cout
        << "PROGRESSION_MGR_INITIALIZATION=OK\n"
        << "PROGRESSION_MGR_VALIDATION=OK\n"
        << "PROGRESSION_MGR_NOCHANGE_EVALUATION=OK\n"
        << "PROGRESSION_MGR_FORWARD_EVALUATION=OK\n"
        << "PROGRESSION_MGR_INVALID_TARGET=OK\n"
        << "PROGRESSION_MGR_ROLLBACK_POLICY=OK\n"
        << "PROGRESSION_MGR_SHARED_BOUNDARY=OK\n"
        << "PROGRESSION_MGR_READ_ONLY=OK\n";

    return 0;
}
