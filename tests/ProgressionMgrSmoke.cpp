#include "ProgressionMgr.h"

#include <iostream>
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
        GPA::ProgressionState const* state,
        std::uint8_t phase,
        std::uint8_t contentStage)
    {
        return
            state &&
            state->phase == phase &&
            state->contentStage == contentStage;
    }
}

int main()
{
    /*
     * --------------------------------------------------
     * NOT INITIALIZED
     * --------------------------------------------------
     */
    GPA::ProgressionMgr manager;

    if (!Expect(
            !manager.IsInitialized(),
            "Manager must start uninitialized.") ||
        !Expect(
            manager.GetActiveState() == nullptr,
            "Uninitialized manager exposed active state.") ||
        !Expect(
            manager.GetActiveDefinition() == nullptr,
            "Uninitialized manager exposed active definition."))
    {
        return 1;
    }

    GPA::TransitionResult beforeInit =
        manager.EvaluateTransition({20, 0});

    if (!Expect(
            beforeInit.status ==
                GPA::TransitionStatus::NotInitialized,
            "Evaluate before initialization must fail.") ||
        !Expect(
            !beforeInit.previous.has_value(),
            "Uninitialized transition invented previous state.") ||
        !Expect(
            !beforeInit.Succeeded(),
            "NotInitialized must not be successful.") ||
        !Expect(
            !beforeInit.Changed(),
            "Evaluation must not report a change."))
    {
        return 2;
    }

    /*
     * --------------------------------------------------
     * INVALID INITIAL STATE
     * --------------------------------------------------
     */
    std::string error;

    if (!Expect(
            !manager.Initialize(
                {20, 1},
                false,
                error),
            "Invalid initial state was accepted.") ||
        !Expect(
            !error.empty(),
            "Invalid initialization returned no error.") ||
        !Expect(
            !manager.IsInitialized(),
            "Failed initialization changed manager state."))
    {
        return 3;
    }

    /*
     * --------------------------------------------------
     * VALID INITIALIZATION
     * --------------------------------------------------
     */
    error.clear();

    if (!Expect(
            manager.Initialize(
                {60, 0},
                false,
                error),
            "Valid initialization failed.") ||
        !Expect(
            error.empty(),
            "Successful initialization returned an error.") ||
        !Expect(
            manager.IsInitialized(),
            "Manager did not become initialized.") ||
        !Expect(
            !manager.IsRollbackAllowed(),
            "Rollback unexpectedly enabled.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                0),
            "Initial state mismatch."))
    {
        return 4;
    }

    GPA::PhaseDefinition const* definition =
        manager.GetActiveDefinition();

    if (!Expect(
            definition != nullptr,
            "Active definition missing.") ||
        !Expect(
            definition->phase == 60,
            "Active definition phase mismatch.") ||
        !Expect(
            definition->contentStageMin == 0 &&
            definition->contentStageMax == 8,
            "Active definition ContentStage range mismatch."))
    {
        return 5;
    }

    /*
     * A second Initialize must never replace authority.
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
                0),
            "Second initialization changed active state."))
    {
        return 6;
    }

    /*
     * --------------------------------------------------
     * NO-OP
     * --------------------------------------------------
     */
    GPA::TransitionResult noChange =
        manager.RequestTransition({60, 0});

    if (!Expect(
            noChange.status ==
                GPA::TransitionStatus::NoChange,
            "No-op returned wrong status.") ||
        !Expect(
            noChange.direction ==
                GPA::TransitionDirection::NoChange,
            "No-op returned wrong direction.") ||
        !Expect(
            noChange.Succeeded(),
            "No-op should be successful.") ||
        !Expect(
            !noChange.Changed(),
            "No-op reported a state change."))
    {
        return 7;
    }

    /*
     * --------------------------------------------------
     * EVALUATION IS NON-DESTRUCTIVE
     * --------------------------------------------------
     */
    GPA::TransitionResult evaluation =
        manager.EvaluateTransition({60, 1});

    if (!Expect(
            evaluation.status ==
                GPA::TransitionStatus::Allowed,
            "Valid forward evaluation was not allowed.") ||
        !Expect(
            evaluation.direction ==
                GPA::TransitionDirection::Forward,
            "Forward evaluation returned wrong direction.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                0),
            "EvaluateTransition modified active state."))
    {
        return 8;
    }

    /*
     * --------------------------------------------------
     * FORWARD CONTENT STAGE
     * --------------------------------------------------
     */
    GPA::TransitionResult forward =
        manager.RequestTransition({60, 1});

    if (!Expect(
            forward.status ==
                GPA::TransitionStatus::Applied,
            "Forward transition was not applied.") ||
        !Expect(
            forward.direction ==
                GPA::TransitionDirection::Forward,
            "Forward transition direction mismatch.") ||
        !Expect(
            forward.previous.has_value(),
            "Applied transition has no previous state.") ||
        !Expect(
            forward.previous->phase == 60 &&
            forward.previous->contentStage == 0,
            "Previous state mismatch.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "Forward transition did not update authority."))
    {
        return 9;
    }

    /*
     * --------------------------------------------------
     * INVALID TARGET MUST NOT MUTATE STATE
     * --------------------------------------------------
     */
    GPA::TransitionResult invalid =
        manager.RequestTransition({60, 9});

    if (!Expect(
            invalid.status ==
                GPA::TransitionStatus::InvalidTarget,
            "Invalid target returned wrong status.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "Invalid target modified active state."))
    {
        return 10;
    }

    /*
     * --------------------------------------------------
     * ROLLBACK BLOCKED
     * --------------------------------------------------
     */
    GPA::TransitionResult blockedRollback =
        manager.RequestTransition({60, 0});

    if (!Expect(
            blockedRollback.status ==
                GPA::TransitionStatus::RollbackNotAllowed,
            "Blocked rollback returned wrong status.") ||
        !Expect(
            blockedRollback.direction ==
                GPA::TransitionDirection::Rollback,
            "Blocked rollback direction mismatch.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                60,
                1),
            "Blocked rollback modified active state."))
    {
        return 11;
    }

    /*
     * --------------------------------------------------
     * SHARED BOUNDARY FORWARD TRANSITION
     * --------------------------------------------------
     */
    GPA::TransitionResult expansion =
        manager.RequestTransition({70, 8});

    if (!Expect(
            expansion.status ==
                GPA::TransitionStatus::Applied,
            "60 -> 70 boundary transition failed.") ||
        !Expect(
            expansion.direction ==
                GPA::TransitionDirection::Forward,
            "60 -> 70 boundary direction mismatch.") ||
        !Expect(
            StateEquals(
                manager.GetActiveState(),
                70,
                8),
            "70/8 did not become active."))
    {
        return 12;
    }

    /*
     * --------------------------------------------------
     * ROLLBACK ENABLED MANAGER
     * --------------------------------------------------
     */
    GPA::ProgressionMgr rollbackManager;

    error.clear();

    if (!Expect(
            rollbackManager.Initialize(
                {80, 13},
                true,
                error),
            "Rollback manager initialization failed.") ||
        !Expect(
            rollbackManager.IsRollbackAllowed(),
            "Rollback manager did not enable rollback."))
    {
        return 13;
    }

    GPA::TransitionResult phaseRollback =
        rollbackManager.RequestTransition({70, 13});

    if (!Expect(
            phaseRollback.status ==
                GPA::TransitionStatus::Applied,
            "Phase rollback was not applied.") ||
        !Expect(
            phaseRollback.direction ==
                GPA::TransitionDirection::Rollback,
            "Phase rollback direction mismatch.") ||
        !Expect(
            StateEquals(
                rollbackManager.GetActiveState(),
                70,
                13),
            "Phase rollback state mismatch."))
    {
        return 14;
    }

    GPA::TransitionResult fullRollback =
        rollbackManager.RequestTransition({60, 8});

    if (!Expect(
            fullRollback.status ==
                GPA::TransitionStatus::Applied,
            "Full rollback was not applied.") ||
        !Expect(
            fullRollback.direction ==
                GPA::TransitionDirection::Rollback,
            "Full rollback direction mismatch.") ||
        !Expect(
            StateEquals(
                rollbackManager.GetActiveState(),
                60,
                8),
            "Full rollback state mismatch."))
    {
        return 15;
    }

    std::cout
        << "PROGRESSION_MGR_INITIALIZATION=OK\n"
        << "PROGRESSION_MGR_NOOP=OK\n"
        << "PROGRESSION_MGR_EVALUATION=OK\n"
        << "PROGRESSION_MGR_FORWARD=OK\n"
        << "PROGRESSION_MGR_INVALID_TARGET=OK\n"
        << "PROGRESSION_MGR_ROLLBACK_BLOCK=OK\n"
        << "PROGRESSION_MGR_SHARED_BOUNDARY=OK\n"
        << "PROGRESSION_MGR_ROLLBACK_ENABLED=OK\n";

    return 0;
}
