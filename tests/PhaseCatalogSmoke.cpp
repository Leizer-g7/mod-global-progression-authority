#include "PhaseCatalog.h"

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

    bool CheckDefinition(
        GPA::PhaseCatalog const& catalog,
        std::uint8_t phase,
        std::uint8_t stageMin,
        std::uint8_t stageMax,
        std::uint8_t playerMaxLevel,
        std::uint16_t crafting,
        std::uint16_t gathering,
        std::uint16_t secondary,
        std::uint16_t riding,
        bool outland,
        bool northrend,
        bool deathKnight)
    {
        GPA::PhaseDefinition const* definition =
            catalog.Find(phase);

        if (!Expect(
                definition != nullptr,
                "Expected phase definition is missing."))
        {
            return false;
        }

        return
            Expect(
                definition->phase == phase,
                "Phase id mismatch.") &&
            Expect(
                definition->contentStageMin == stageMin,
                "ContentStage minimum mismatch.") &&
            Expect(
                definition->contentStageMax == stageMax,
                "ContentStage maximum mismatch.") &&
            Expect(
                definition->playerMaxLevel == playerMaxLevel,
                "Player max level mismatch.") &&
            Expect(
                definition->professions.craftingCap == crafting,
                "Crafting cap mismatch.") &&
            Expect(
                definition->professions.gatheringCap == gathering,
                "Gathering cap mismatch.") &&
            Expect(
                definition->professions.secondaryCap == secondary,
                "Secondary profession cap mismatch.") &&
            Expect(
                definition->professions.ridingCap == riding,
                "Riding cap mismatch.") &&
            Expect(
                definition->world.outlandEnabled == outland,
                "Outland policy mismatch.") &&
            Expect(
                definition->world.northrendEnabled == northrend,
                "Northrend policy mismatch.") &&
            Expect(
                definition->deathKnightEnabled == deathKnight,
                "Death Knight policy mismatch.");
    }
}

int main()
{
    GPA::PhaseCatalog catalog;

    std::string validationError;

    if (!Expect(
            catalog.Validate(validationError),
            validationError.empty()
                ? "Catalog validation failed."
                : validationError.c_str()))
    {
        return 1;
    }

    std::vector<std::uint8_t> const expectedPhases =
    {
        20,
        30,
        40,
        50,
        60,
        70,
        80
    };

    if (!Expect(
            catalog.GetPhaseIds() == expectedPhases,
            "Phase id list mismatch."))
    {
        return 2;
    }

    if (!Expect(
            !catalog.HasPhase(10),
            "Unexpected Phase 10 exists.") ||
        !Expect(
            !catalog.HasPhase(90),
            "Unexpected Phase 90 exists."))
    {
        return 3;
    }

    if (!CheckDefinition(
            catalog,
            20,
            0,
            0,
            20,
            150,
            150,
            150,
            75,
            false,
            false,
            false))
    {
        return 10;
    }

    if (!CheckDefinition(
            catalog,
            30,
            0,
            0,
            30,
            225,
            225,
            225,
            75,
            false,
            false,
            false))
    {
        return 11;
    }

    if (!CheckDefinition(
            catalog,
            40,
            0,
            0,
            40,
            300,
            300,
            300,
            150,
            false,
            false,
            false))
    {
        return 12;
    }

    if (!CheckDefinition(
            catalog,
            50,
            0,
            0,
            50,
            300,
            300,
            300,
            150,
            false,
            false,
            false))
    {
        return 13;
    }

    if (!CheckDefinition(
            catalog,
            60,
            0,
            8,
            60,
            300,
            300,
            300,
            150,
            false,
            false,
            false))
    {
        return 14;
    }

    if (!CheckDefinition(
            catalog,
            70,
            8,
            13,
            70,
            375,
            375,
            375,
            300,
            true,
            false,
            false))
    {
        return 15;
    }

    if (!CheckDefinition(
            catalog,
            80,
            13,
            18,
            80,
            450,
            450,
            450,
            300,
            true,
            true,
            true))
    {
        return 16;
    }

    /*
     * Exact state-validation parity checks.
     */
    struct StateCase
    {
        GPA::ProgressionState state;
        bool expected;
    };

    std::vector<StateCase> const stateCases =
    {
        {{20, 0}, true},
        {{20, 1}, false},

        {{30, 0}, true},
        {{30, 1}, false},

        {{40, 0}, true},
        {{50, 0}, true},

        {{60, 0}, true},
        {{60, 1}, true},
        {{60, 8}, true},
        {{60, 9}, false},

        {{70, 7}, false},
        {{70, 8}, true},
        {{70, 13}, true},
        {{70, 14}, false},

        {{80, 12}, false},
        {{80, 13}, true},
        {{80, 18}, true},
        {{80, 19}, false},

        {{10, 0}, false},
        {{90, 18}, false}
    };

    for (StateCase const& test : stateCases)
    {
        bool const actual =
            catalog.IsValidState(
                test.state);

        if (actual != test.expected)
        {
            std::cerr
                << "FAIL: state Phase="
                << static_cast<unsigned>(
                    test.state.phase)
                << " ContentStage="
                << static_cast<unsigned>(
                    test.state.contentStage)
                << " expected="
                << test.expected
                << " actual="
                << actual
                << '\n';

            return 20;
        }
    }

    /*
     * Shared historical handoff boundary.
     */
    if (!Expect(
            catalog.IsValidState({60, 8}),
            "Phase 60 / Stage 8 must be valid.") ||
        !Expect(
            catalog.IsValidState({70, 8}),
            "Phase 70 / Stage 8 must be valid.") ||
        !Expect(
            catalog.IsValidState({70, 13}),
            "Phase 70 / Stage 13 must be valid.") ||
        !Expect(
            catalog.IsValidState({80, 13}),
            "Phase 80 / Stage 13 must be valid."))
    {
        return 21;
    }

    std::cout
        << "PHASE_CATALOG_VALIDATE=OK\n"
        << "PHASE_CATALOG_DEFINITIONS=7\n"
        << "PHASE_CATALOG_STATE_MATRIX=OK\n"
        << "PHASE_CATALOG_SHARED_BOUNDARIES=OK\n";

    return 0;
}
