#include "EconomyPolicyCatalog.h"
#include "PhaseCatalog.h"
#include "PlayerbotsPolicyCatalog.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

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

    bool CheckPlayerbots(
        GPA::PlayerbotsPolicyCatalog const& catalog,
        std::uint8_t phase,
        std::uint8_t maxLevel,
        std::uint32_t gearMaxItemLevel,
        char const* distribution)
    {
        GPA::PlayerbotsPhasePolicy const* policy =
            catalog.Find(phase);

        if (!Expect(
                policy != nullptr,
                "Missing Playerbots phase policy."))
        {
            return false;
        }

        return
            Expect(
                policy->maxLevel == maxLevel,
                "Playerbots max level mismatch.") &&
            Expect(
                policy->gearMaxItemLevel ==
                    gearMaxItemLevel,
                "Playerbots gear item level mismatch.") &&
            Expect(
                policy->distribution ==
                    distribution,
                "Playerbots distribution mismatch.");
    }

    bool CheckEconomy(
        GPA::EconomyPolicyCatalog const& catalog,
        std::uint8_t phase,
        std::uint32_t materialMaxItemLevel)
    {
        GPA::EconomyPhasePolicy const* policy =
            catalog.Find(phase);

        if (!Expect(
                policy != nullptr,
                "Missing economy phase policy."))
        {
            return false;
        }

        return Expect(
            policy->materialMaxItemLevel ==
                materialMaxItemLevel,
            "Economy material item level mismatch.");
    }
}

int main()
{
    GPA::PhaseCatalog coreCatalog;
    GPA::PlayerbotsPolicyCatalog playerbotsCatalog;
    GPA::EconomyPolicyCatalog economyCatalog;

    std::string error;

    if (!Expect(
            coreCatalog.Validate(error),
            error.empty()
                ? "Core catalog validation failed."
                : error.c_str()))
    {
        return 1;
    }

    error.clear();

    if (!Expect(
            playerbotsCatalog.Validate(error),
            error.empty()
                ? "Playerbots catalog validation failed."
                : error.c_str()))
    {
        return 2;
    }

    error.clear();

    if (!Expect(
            economyCatalog.Validate(error),
            error.empty()
                ? "Economy catalog validation failed."
                : error.c_str()))
    {
        return 3;
    }

    std::vector<std::uint8_t> const corePhases =
        coreCatalog.GetPhaseIds();

    if (!Expect(
            playerbotsCatalog.GetPhaseIds() ==
                corePhases,
            "Playerbots phase coverage differs from Core."))
    {
        return 4;
    }

    if (!Expect(
            economyCatalog.GetPhaseIds() ==
                corePhases,
            "Economy phase coverage differs from Core."))
    {
        return 5;
    }

    if (!Expect(
            !playerbotsCatalog.HasPhase(10),
            "Unexpected Playerbots Phase 10 exists.") ||
        !Expect(
            !economyCatalog.HasPhase(90),
            "Unexpected Economy Phase 90 exists."))
    {
        return 6;
    }

    if (!CheckPlayerbots(
            playerbotsCatalog,
            20,
            20,
            25,
            "1-9:15,10-19:50,20:35") ||
        !CheckPlayerbots(
            playerbotsCatalog,
            30,
            30,
            35,
            "1-9:10,10-19:15,20-29:45,30:30") ||
        !CheckPlayerbots(
            playerbotsCatalog,
            40,
            40,
            45,
            "1-9:8,10-19:10,20-29:15,30-39:42,40:25") ||
        !CheckPlayerbots(
            playerbotsCatalog,
            50,
            50,
            55,
            "1-9:6,10-19:8,20-29:10,30-39:12,40-49:40,50:24") ||
        !CheckPlayerbots(
            playerbotsCatalog,
            60,
            60,
            78,
            "1-9:5,10-19:7,20-29:8,30-39:10,40-49:12,50-59:38,60:20") ||
        !CheckPlayerbots(
            playerbotsCatalog,
            70,
            70,
            125,
            "1-9:4,10-19:5,20-29:6,30-39:7,40-49:8,50-59:10,60-69:40,70:20") ||
        !CheckPlayerbots(
            playerbotsCatalog,
            80,
            80,
            224,
            "1-9:3,10-19:4,20-29:5,30-39:6,40-49:7,50-59:8,60-69:10,70-79:37,80:20"))
    {
        return 10;
    }

    if (!CheckEconomy(
            economyCatalog,
            20,
            20) ||
        !CheckEconomy(
            economyCatalog,
            30,
            30) ||
        !CheckEconomy(
            economyCatalog,
            40,
            40) ||
        !CheckEconomy(
            economyCatalog,
            50,
            50) ||
        !CheckEconomy(
            economyCatalog,
            60,
            55) ||
        !CheckEconomy(
            economyCatalog,
            70,
            70) ||
        !CheckEconomy(
            economyCatalog,
            80,
            999))
    {
        return 11;
    }

    /*
     * During V1 parity Playerbots max level remains explicit,
     * but all current policies must still match the global
     * player level cap.
     */
    for (std::uint8_t phase : corePhases)
    {
        GPA::PhaseDefinition const* core =
            coreCatalog.Find(phase);

        GPA::PlayerbotsPhasePolicy const* playerbots =
            playerbotsCatalog.Find(phase);

        if (!core || !playerbots)
        {
            std::cerr
                << "FAIL: phase coverage unexpectedly missing.\n";

            return 12;
        }

        if (core->playerMaxLevel !=
            playerbots->maxLevel)
        {
            std::cerr
                << "FAIL: Phase "
                << static_cast<unsigned>(phase)
                << " player cap and Playerbots cap diverge.\n";

            return 13;
        }
    }

    std::cout
        << "ADAPTER_POLICY_CATALOGS_VALIDATE=OK\n"
        << "PLAYERBOTS_POLICY_PHASES=7\n"
        << "ECONOMY_POLICY_PHASES=7\n"
        << "POLICY_PHASE_COVERAGE=OK\n"
        << "PLAYERBOTS_PARITY_DATA=OK\n"
        << "ECONOMY_PARITY_DATA=OK\n"
        << "PLAYERBOT_CAP_ALIGNMENT=OK\n";

    return 0;
}
