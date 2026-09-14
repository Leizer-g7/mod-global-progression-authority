#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_ECONOMY_POLICY_CATALOG_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_ECONOMY_POLICY_CATALOG_H

#include "AdapterPolicies.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace GPA
{
    class EconomyPolicyCatalog
    {
    public:
        EconomyPolicyCatalog();

        bool Validate(std::string& error) const;

        bool HasPhase(std::uint8_t phase) const;

        EconomyPhasePolicy const* Find(
            std::uint8_t phase) const;

        std::vector<std::uint8_t> GetPhaseIds() const;

    private:
        std::map<std::uint8_t, EconomyPhasePolicy> _policies;
    };
}

#endif
