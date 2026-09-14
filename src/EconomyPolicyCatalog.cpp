#include "EconomyPolicyCatalog.h"

namespace GPA
{
    EconomyPolicyCatalog::EconomyPolicyCatalog()
    {
        _policies.emplace(
            20,
            EconomyPhasePolicy{20});

        _policies.emplace(
            30,
            EconomyPhasePolicy{30});

        _policies.emplace(
            40,
            EconomyPhasePolicy{40});

        _policies.emplace(
            50,
            EconomyPhasePolicy{50});

        _policies.emplace(
            60,
            EconomyPhasePolicy{55});

        _policies.emplace(
            70,
            EconomyPhasePolicy{70});

        _policies.emplace(
            80,
            EconomyPhasePolicy{999});
    }

    bool EconomyPolicyCatalog::Validate(
        std::string& error) const
    {
        if (_policies.empty())
        {
            error =
                "Economy policy catalog is empty.";

            return false;
        }

        for (auto const& entry : _policies)
        {
            std::uint8_t const phase =
                entry.first;

            EconomyPhasePolicy const& policy =
                entry.second;

            if (!phase)
            {
                error =
                    "Economy policy has invalid phase id.";

                return false;
            }

            if (!policy.materialMaxItemLevel)
            {
                error =
                    "Economy material max item level cannot be zero.";

                return false;
            }
        }

        error.clear();
        return true;
    }

    bool EconomyPolicyCatalog::HasPhase(
        std::uint8_t phase) const
    {
        return
            _policies.find(phase) !=
            _policies.end();
    }

    EconomyPhasePolicy const*
    EconomyPolicyCatalog::Find(
        std::uint8_t phase) const
    {
        auto const itr =
            _policies.find(phase);

        if (itr == _policies.end())
            return nullptr;

        return &itr->second;
    }

    std::vector<std::uint8_t>
    EconomyPolicyCatalog::GetPhaseIds() const
    {
        std::vector<std::uint8_t> result;

        result.reserve(
            _policies.size());

        for (auto const& entry : _policies)
            result.push_back(entry.first);

        return result;
    }
}
