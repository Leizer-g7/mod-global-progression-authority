#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PHASE_CATALOG_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PHASE_CATALOG_H

#include "PhaseDefinition.h"
#include "ProgressionState.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace GPA
{
    class PhaseCatalog
    {
    public:
        PhaseCatalog();

        bool Validate(std::string& error) const;

        bool HasPhase(std::uint8_t phase) const;

        PhaseDefinition const* Find(
            std::uint8_t phase) const;

        bool IsValidContentStage(
            std::uint8_t phase,
            std::uint8_t contentStage) const;

        bool IsValidState(
            ProgressionState const& state) const;

        std::vector<std::uint8_t> GetPhaseIds() const;

    private:
        std::map<std::uint8_t, PhaseDefinition> _definitions;
    };
}

#endif
