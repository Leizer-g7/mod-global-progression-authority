#include "DeathKnightPullPolicy.h"

#include <iostream>

namespace
{
    int failures = 0;

    void Check(
        bool condition,
        char const* name)
    {
        if (condition)
        {
            std::cout
                << "PASS: "
                << name
                << '\n';

            return;
        }

        std::cout
            << "FAIL: "
            << name
            << '\n';

        ++failures;
    }

    bool IsAllowed(
        GPA::DeathKnightCreationDecision decision)
    {
        return
            decision ==
            GPA::DeathKnightCreationDecision::Allow;
    }
}

int main()
{
    using GPA::ClassifyDeathKnightCreationPolicy;
    using GPA::PullPolicyStatus;

    Check(
        IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::Unavailable,
                false,
                false,
                false)),
        "Unavailable leaves non-DK creation unmanaged.");

    Check(
        IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::Disabled,
                false,
                false,
                true)),
        "Disabled progression allows DK creation.");

    Check(
        !IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::Ready,
                true,
                false,
                true)),
        "Ready P20-style policy blocks DK creation.");

    Check(
        IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::Ready,
                true,
                true,
                true)),
        "Ready P80-style policy allows DK creation.");

    Check(
        !IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::ActiveFailClosed,
                true,
                false,
                true)),
        "ActiveFailClosed preserves known DK lock.");

    Check(
        IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::ActiveFailClosed,
                true,
                true,
                true)),
        "ActiveFailClosed preserves known DK unlock.");

    Check(
        !IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::Unavailable,
                false,
                false,
                true)),
        "Unavailable authority blocks DK creation.");

    Check(
        !IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::Ready,
                false,
                false,
                true)),
        "Ready without definition fails closed.");

    Check(
        !IsAllowed(
            ClassifyDeathKnightCreationPolicy(
                PullPolicyStatus::ActiveFailClosed,
                false,
                false,
                true)),
        "ActiveFailClosed without definition fails closed.");

    std::cout
        << "DEATH_KNIGHT_PULL_POLICY_SMOKE="
        << (failures == 0 ? "PASS" : "FAIL")
        << '\n';

    return failures == 0 ? 0 : 1;
}
