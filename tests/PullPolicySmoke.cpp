#include "PullPolicy.h"

#include <iostream>

namespace
{
    bool Expect(
        bool condition,
        char const* label)
    {
        if (condition)
        {
            std::cout
                << "PASS: "
                << label
                << '\n';

            return true;
        }

        std::cerr
            << "FAIL: "
            << label
            << '\n';

        return false;
    }
}

int main()
{
    bool ok = true;

    using GPA::ClassifyPullPolicy;
    using GPA::PullPolicySnapshot;
    using GPA::PullPolicyStatus;

    ok &=
        Expect(
            ClassifyPullPolicy(
                false,
                false,
                false,
                false,
                false) ==
                PullPolicyStatus::Unavailable,
            "config-not-loaded is unavailable");

    ok &=
        Expect(
            ClassifyPullPolicy(
                true,
                false,
                false,
                false,
                false) ==
                PullPolicyStatus::Disabled,
            "disabled GPA bypasses pull policy");

    ok &=
        Expect(
            ClassifyPullPolicy(
                true,
                true,
                false,
                true,
                true) ==
                PullPolicyStatus::Unavailable,
            "invalid enabled config is unavailable");

    ok &=
        Expect(
            ClassifyPullPolicy(
                true,
                true,
                true,
                false,
                true) ==
                PullPolicyStatus::Unavailable,
            "enabled GPA without active definition is unavailable");

    ok &=
        Expect(
            ClassifyPullPolicy(
                true,
                true,
                true,
                true,
                false) ==
                PullPolicyStatus::ActiveFailClosed,
            "active unpublished authority is fail-closed");

    ok &=
        Expect(
            ClassifyPullPolicy(
                true,
                true,
                true,
                true,
                true) ==
                PullPolicyStatus::Ready,
            "published active authority is ready");

    PullPolicySnapshot disabled;
    disabled.status =
        PullPolicyStatus::Disabled;

    ok &=
        Expect(
            !disabled.HasActivePolicy(),
            "disabled snapshot has no active policy");

    PullPolicySnapshot ready;
    ready.status =
        PullPolicyStatus::Ready;
    ready.definition =
        GPA::PhaseDefinition{};

    ok &=
        Expect(
            ready.HasActivePolicy() &&
            !ready.IsFailClosed(),
            "ready snapshot exposes active policy");

    PullPolicySnapshot failClosed;
    failClosed.status =
        PullPolicyStatus::ActiveFailClosed;
    failClosed.definition =
        GPA::PhaseDefinition{};

    ok &=
        Expect(
            failClosed.HasActivePolicy() &&
            failClosed.IsFailClosed(),
            "active unpublished snapshot keeps policy fail-closed");

    PullPolicySnapshot unavailable;
    unavailable.status =
        PullPolicyStatus::Unavailable;

    ok &=
        Expect(
            !unavailable.HasActivePolicy() &&
            unavailable.IsFailClosed(),
            "unavailable snapshot is fail-closed without policy");

    if (!ok)
        return 1;

    std::cout
        << "PULL_POLICY_SMOKE=PASS\n";

    return 0;
}
