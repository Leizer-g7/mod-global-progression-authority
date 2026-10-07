#include "ProfessionPullPolicy.h"

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
                << "PASS " << name << '\n';

            return;
        }

        ++failures;

        std::cout
            << "FAIL " << name << '\n';
    }
}

int main()
{
    GPA::PhaseDefinition definition;

    definition.professions.craftingCap = 150;
    definition.professions.gatheringCap = 150;
    definition.professions.secondaryCap = 150;
    definition.professions.ridingCap = 75;

    Check(
        GPA::GetProfessionCap(
            definition,
            164) == 150,
        "crafting-blacksmithing");

    Check(
        GPA::GetProfessionCap(
            definition,
            773) == 150,
        "crafting-inscription");

    Check(
        GPA::GetProfessionCap(
            definition,
            182) == 150,
        "gathering-herbalism");

    Check(
        GPA::GetProfessionCap(
            definition,
            393) == 150,
        "gathering-skinning");

    Check(
        GPA::GetProfessionCap(
            definition,
            129) == 150,
        "secondary-first-aid");

    Check(
        GPA::GetProfessionCap(
            definition,
            356) == 150,
        "secondary-fishing");

    Check(
        GPA::GetProfessionCap(
            definition,
            762) == 75,
        "riding");

    Check(
        GPA::GetProfessionCap(
            definition,
            43) == 0,
        "unmanaged-skill");

    Check(
        GPA::GetMaxAllowedProfessionStep(75) == 1,
        "step-75");

    Check(
        GPA::GetMaxAllowedProfessionStep(150) == 2,
        "step-150");

    Check(
        GPA::GetMaxAllowedProfessionStep(225) == 3,
        "step-225");

    Check(
        GPA::GetMaxAllowedProfessionStep(300) == 4,
        "step-300");

    Check(
        GPA::GetMaxAllowedProfessionStep(375) == 5,
        "step-375");

    Check(
        GPA::GetMaxAllowedProfessionStep(450) == 6,
        "step-450");

    GPA::ProfessionPullDecision known =
        GPA::BuildProfessionPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &definition,
            164);

    Check(
        known.managed &&
        known.mode ==
            GPA::PlayerPullPolicyMode::EnforceKnownCap &&
        known.cap == 150,
        "known-policy-cap");

    GPA::ProfessionPullDecision riding =
        GPA::BuildProfessionPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            &definition,
            762);

    Check(
        riding.managed &&
        riding.cap == 75,
        "known-riding-cap");

    GPA::ProfessionPullDecision failClosed =
        GPA::BuildProfessionPullDecision(
            GPA::PlayerPullPolicyMode::BlockProgression,
            nullptr,
            164);

    Check(
        failClosed.managed &&
        failClosed.mode ==
            GPA::PlayerPullPolicyMode::BlockProgression &&
        failClosed.cap == 0,
        "fail-closed-managed");

    GPA::ProfessionPullDecision missingDefinition =
        GPA::BuildProfessionPullDecision(
            GPA::PlayerPullPolicyMode::EnforceKnownCap,
            nullptr,
            164);

    Check(
        missingDefinition.managed &&
        missingDefinition.mode ==
            GPA::PlayerPullPolicyMode::BlockProgression &&
        missingDefinition.cap == 0,
        "known-without-definition-fails-closed");

    GPA::ProfessionPullDecision bypass =
        GPA::BuildProfessionPullDecision(
            GPA::PlayerPullPolicyMode::Bypass,
            &definition,
            164);

    Check(
        bypass.managed &&
        bypass.mode ==
            GPA::PlayerPullPolicyMode::Bypass &&
        bypass.cap == 0,
        "bypass");

    GPA::ProfessionPullDecision unrelated =
        GPA::BuildProfessionPullDecision(
            GPA::PlayerPullPolicyMode::BlockProgression,
            nullptr,
            43);

    Check(
        !unrelated.managed &&
        unrelated.cap == 0,
        "fail-closed-does-not-capture-unrelated-skills");

    constexpr std::uint32_t managedSkills[] =
    {
        129, 185, 356,
        164, 165, 171, 197, 202, 333, 755, 773,
        182, 186, 393,
        762
    };

    for (std::uint32_t skillId :
         managedSkills)
    {
        Check(
            GPA::IsManagedProfessionSkill(skillId),
            "managed-skill-list");
    }

    std::cout
        << "FAILURES=" << failures << '\n';

    if (failures)
        return 1;

    std::cout
        << "H3D3G7_B3_A2_PROFESSION_POLICY_SMOKE=PASS\n";

    return 0;
}
