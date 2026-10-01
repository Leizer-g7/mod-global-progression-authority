#include "PlayerPullPolicy.h"

#include <iostream>

namespace
{
    bool Expect(
        char const* name,
        GPA::PlayerPullPolicyMode actual,
        GPA::PlayerPullPolicyMode expected)
    {
        bool const ok =
            actual == expected;

        std::cout
            << name
            << "="
            << (ok ? "PASS" : "FAIL")
            << '\n';

        return ok;
    }
}

int main()
{
    using GPA::ClassifyPlayerPullPolicy;
    using GPA::PlayerPullPolicyMode;
    using GPA::PullPolicyStatus;

    bool ok = true;

    ok &= Expect(
        "disabled",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::Disabled,
            false,
            false,
            false),
        PlayerPullPolicyMode::Bypass);

    ok &= Expect(
        "ready_known_cap",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::Ready,
            true,
            false,
            false),
        PlayerPullPolicyMode::EnforceKnownCap);

    ok &= Expect(
        "active_failclosed_known_cap",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::ActiveFailClosed,
            true,
            false,
            false),
        PlayerPullPolicyMode::EnforceKnownCap);

    ok &= Expect(
        "unavailable",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::Unavailable,
            false,
            false,
            false),
        PlayerPullPolicyMode::BlockProgression);

    ok &= Expect(
        "ready_without_definition",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::Ready,
            false,
            false,
            false),
        PlayerPullPolicyMode::BlockProgression);

    ok &= Expect(
        "gm_bypass_ready",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::Ready,
            true,
            true,
            true),
        PlayerPullPolicyMode::Bypass);

    ok &= Expect(
        "gm_bypass_unavailable",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::Unavailable,
            false,
            true,
            true),
        PlayerPullPolicyMode::Bypass);

    ok &= Expect(
        "gm_bypass_disabled_for_gm",
        ClassifyPlayerPullPolicy(
            PullPolicyStatus::Unavailable,
            false,
            false,
            true),
        PlayerPullPolicyMode::BlockProgression);

    std::cout
        << "PLAYER_PULL_POLICY_SMOKE="
        << (ok ? "PASS" : "FAIL")
        << '\n';

    return ok ? 0 : 1;
}
