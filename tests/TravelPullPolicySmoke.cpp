#include "TravelPullPolicy.h"

#include <cstdint>
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

    GPA::PhaseDefinition MakeDefinition(
        bool outland,
        bool northrend)
    {
        GPA::PhaseDefinition definition;

        definition.world.outlandEnabled = outland;
        definition.world.northrendEnabled = northrend;

        return definition;
    }

    GPA::TravelTarget OutlandDirect()
    {
        return GPA::TravelTarget{
            GPA::TravelExpansion::Outland,
            true,
            false
        };
    }

    GPA::TravelTarget OutlandInstance()
    {
        return GPA::TravelTarget{
            GPA::TravelExpansion::Outland,
            false,
            false
        };
    }

    GPA::TravelTarget TbcStarterZone()
    {
        return GPA::TravelTarget{
            GPA::TravelExpansion::Outland,
            true,
            true
        };
    }

    GPA::TravelTarget NorthrendDirect()
    {
        return GPA::TravelTarget{
            GPA::TravelExpansion::Northrend,
            true,
            false
        };
    }

    GPA::TravelTarget NorthrendInstance()
    {
        return GPA::TravelTarget{
            GPA::TravelExpansion::Northrend,
            false,
            false
        };
    }

    GPA::TravelTarget Unmanaged()
    {
        return GPA::TravelTarget{
            GPA::TravelExpansion::None,
            false,
            false
        };
    }
}

int main()
{
    GPA::PhaseDefinition p20 =
        MakeDefinition(false, false);

    GPA::PhaseDefinition p70 =
        MakeDefinition(true, false);

    GPA::PhaseDefinition p80 =
        MakeDefinition(true, true);


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::Bypass,
            &p20,
            OutlandDirect(),
            1).IsBlocked(),
        "Bypass allows managed travel.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::BlockProgression,
            nullptr,
            Unmanaged(),
            1).IsBlocked(),
        "Unavailable leaves unmanaged maps alone.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::BlockProgression,
            nullptr,
            TbcStarterZone(),
            1).IsBlocked(),
        "Unavailable preserves TBC starter zones.");


    Check(
        GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::BlockProgression,
            nullptr,
            OutlandDirect(),
            70).restriction ==
            GPA::TravelRestriction::AuthorityUnavailable,
        "Unavailable blocks managed Outland travel.");


    Check(
        GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::BlockProgression,
            nullptr,
            NorthrendInstance(),
            80).restriction ==
            GPA::TravelRestriction::AuthorityUnavailable,
        "Unavailable blocks managed Northrend instances.");


    Check(
        GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p20,
            OutlandDirect(),
            60).restriction ==
            GPA::TravelRestriction::OutlandPhase,
        "P20 blocks Outland by phase.");


    Check(
        GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p70,
            OutlandDirect(),
            57).restriction ==
            GPA::TravelRestriction::OutlandLevel,
        "P70 requires level 58 for direct Outland.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p70,
            OutlandDirect(),
            58).IsBlocked(),
        "P70 level 58 permits direct Outland.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p70,
            OutlandInstance(),
            1).IsBlocked(),
        "Outland instance does not inherit direct level 58 gate.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p20,
            TbcStarterZone(),
            1).IsBlocked(),
        "TBC starter zones bypass closed Outland.");


    Check(
        GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p70,
            NorthrendDirect(),
            80).restriction ==
            GPA::TravelRestriction::NorthrendPhase,
        "P70 blocks Northrend by phase.");


    Check(
        GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p80,
            NorthrendDirect(),
            67).restriction ==
            GPA::TravelRestriction::NorthrendLevel,
        "P80 requires level 68 for direct Northrend.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p80,
            NorthrendDirect(),
            68).IsBlocked(),
        "P80 level 68 permits direct Northrend.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p80,
            NorthrendInstance(),
            1).IsBlocked(),
        "Northrend instance does not inherit direct level 68 gate.");


    Check(
        !GPA::BuildTravelPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &p20,
            Unmanaged(),
            1).IsBlocked(),
        "Naxx/unmanaged target remains outside global travel gate.");


    std::cout
        << "FAILURES="
        << failures
        << '\n';

    if (failures != 0)
        return 1;

    std::cout
        << "H3D3G7_B3_A3_TRAVEL_POLICY_SMOKE=PASS"
        << '\n';

    return 0;
}
