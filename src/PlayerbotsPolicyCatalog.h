#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PLAYERBOTS_POLICY_CATALOG_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PLAYERBOTS_POLICY_CATALOG_H

#include "AdapterPolicies.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace GPA
{
    class PlayerbotsPolicyCatalog
    {
    public:
        PlayerbotsPolicyCatalog();

        bool Validate(std::string& error) const;

        bool HasPhase(std::uint8_t phase) const;

        PlayerbotsPhasePolicy const* Find(
            std::uint8_t phase) const;

        std::vector<std::uint8_t> GetPhaseIds() const;

    private:
        std::map<std::uint8_t, PlayerbotsPhasePolicy> _policies;
    };
}

#endif
