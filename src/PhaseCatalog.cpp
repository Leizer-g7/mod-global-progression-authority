#include "PhaseCatalog.h"

namespace GPA
{
    PhaseCatalog::PhaseCatalog()
    {
        _definitions.emplace(
            20,
            PhaseDefinition{
                20,
                0,
                0,
                20,
                ProfessionPolicy{
                    150,
                    150,
                    150,
                    75
                },
                WorldAvailability{
                    false,
                    false
                },
                false
            });

        _definitions.emplace(
            30,
            PhaseDefinition{
                30,
                0,
                0,
                30,
                ProfessionPolicy{
                    225,
                    225,
                    225,
                    75
                },
                WorldAvailability{
                    false,
                    false
                },
                false
            });

        _definitions.emplace(
            40,
            PhaseDefinition{
                40,
                0,
                0,
                40,
                ProfessionPolicy{
                    300,
                    300,
                    300,
                    150
                },
                WorldAvailability{
                    false,
                    false
                },
                false
            });

        _definitions.emplace(
            50,
            PhaseDefinition{
                50,
                0,
                0,
                50,
                ProfessionPolicy{
                    300,
                    300,
                    300,
                    150
                },
                WorldAvailability{
                    false,
                    false
                },
                false
            });

        _definitions.emplace(
            60,
            PhaseDefinition{
                60,
                0,
                8,
                60,
                ProfessionPolicy{
                    300,
                    300,
                    300,
                    150
                },
                WorldAvailability{
                    false,
                    false
                },
                false
            });

        _definitions.emplace(
            70,
            PhaseDefinition{
                70,
                8,
                13,
                70,
                ProfessionPolicy{
                    375,
                    375,
                    375,
                    300
                },
                WorldAvailability{
                    true,
                    false
                },
                false
            });

        _definitions.emplace(
            80,
            PhaseDefinition{
                80,
                13,
                18,
                80,
                ProfessionPolicy{
                    450,
                    450,
                    450,
                    300
                },
                WorldAvailability{
                    true,
                    true
                },
                true
            });
    }

    bool PhaseCatalog::Validate(
        std::string& error) const
    {
        if (_definitions.empty())
        {
            error =
                "Phase catalog is empty.";

            return false;
        }

        for (auto const& entry : _definitions)
        {
            std::uint8_t const key =
                entry.first;

            PhaseDefinition const& definition =
                entry.second;

            if (key != definition.phase)
            {
                error =
                    "Phase catalog key does not match "
                    "PhaseDefinition.phase.";

                return false;
            }

            if (definition.phase == 0)
            {
                error =
                    "Phase id cannot be zero.";

                return false;
            }

            if (definition.contentStageMin >
                definition.contentStageMax)
            {
                error =
                    "Phase has an invalid ContentStage range.";

                return false;
            }

            if (definition.playerMaxLevel == 0)
            {
                error =
                    "Player max level cannot be zero.";

                return false;
            }

            ProfessionPolicy const& professions =
                definition.professions;

            if (!professions.craftingCap ||
                !professions.gatheringCap ||
                !professions.secondaryCap ||
                !professions.ridingCap)
            {
                error =
                    "Profession caps cannot be zero.";

                return false;
            }
        }

        error.clear();
        return true;
    }

    bool PhaseCatalog::HasPhase(
        std::uint8_t phase) const
    {
        return
            _definitions.find(phase) !=
            _definitions.end();
    }

    PhaseDefinition const* PhaseCatalog::Find(
        std::uint8_t phase) const
    {
        auto const itr =
            _definitions.find(phase);

        if (itr == _definitions.end())
            return nullptr;

        return &itr->second;
    }

    bool PhaseCatalog::IsValidContentStage(
        std::uint8_t phase,
        std::uint8_t contentStage) const
    {
        PhaseDefinition const* definition =
            Find(phase);

        if (!definition)
            return false;

        return
            contentStage >=
                definition->contentStageMin &&
            contentStage <=
                definition->contentStageMax;
    }

    bool PhaseCatalog::IsValidState(
        ProgressionState const& state) const
    {
        return IsValidContentStage(
            state.phase,
            state.contentStage);
    }

    std::vector<std::uint8_t>
    PhaseCatalog::GetPhaseIds() const
    {
        std::vector<std::uint8_t> result;

        result.reserve(
            _definitions.size());

        for (auto const& entry : _definitions)
            result.push_back(entry.first);

        return result;
    }
}
