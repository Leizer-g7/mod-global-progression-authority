#include "PlayerbotsPolicyCatalog.h"

namespace GPA
{
    PlayerbotsPolicyCatalog::PlayerbotsPolicyCatalog()
    {
        _policies.emplace(
            20,
            PlayerbotsPhasePolicy{
                20,
                25,
                "1-9:15,10-19:50,20:35"
            });

        _policies.emplace(
            30,
            PlayerbotsPhasePolicy{
                30,
                35,
                "1-9:10,10-19:15,20-29:45,30:30"
            });

        _policies.emplace(
            40,
            PlayerbotsPhasePolicy{
                40,
                45,
                "1-9:8,10-19:10,20-29:15,30-39:42,40:25"
            });

        _policies.emplace(
            50,
            PlayerbotsPhasePolicy{
                50,
                55,
                "1-9:6,10-19:8,20-29:10,30-39:12,40-49:40,50:24"
            });

        _policies.emplace(
            60,
            PlayerbotsPhasePolicy{
                60,
                78,
                "1-9:5,10-19:7,20-29:8,30-39:10,40-49:12,50-59:38,60:20"
            });

        _policies.emplace(
            70,
            PlayerbotsPhasePolicy{
                70,
                125,
                "1-9:4,10-19:5,20-29:6,30-39:7,40-49:8,50-59:10,60-69:40,70:20"
            });

        _policies.emplace(
            80,
            PlayerbotsPhasePolicy{
                80,
                224,
                "1-9:3,10-19:4,20-29:5,30-39:6,40-49:7,50-59:8,60-69:10,70-79:37,80:20"
            });
    }

    bool PlayerbotsPolicyCatalog::Validate(
        std::string& error) const
    {
        if (_policies.empty())
        {
            error =
                "Playerbots policy catalog is empty.";

            return false;
        }

        for (auto const& entry : _policies)
        {
            std::uint8_t const phase =
                entry.first;

            PlayerbotsPhasePolicy const& policy =
                entry.second;

            if (!phase)
            {
                error =
                    "Playerbots policy has invalid phase id.";

                return false;
            }

            if (!policy.maxLevel)
            {
                error =
                    "Playerbots max level cannot be zero.";

                return false;
            }

            if (!policy.gearMaxItemLevel)
            {
                error =
                    "Playerbots gear max item level cannot be zero.";

                return false;
            }

            if (policy.distribution.empty())
            {
                error =
                    "Playerbots distribution cannot be empty.";

                return false;
            }
        }

        error.clear();
        return true;
    }

    bool PlayerbotsPolicyCatalog::HasPhase(
        std::uint8_t phase) const
    {
        return
            _policies.find(phase) !=
            _policies.end();
    }

    PlayerbotsPhasePolicy const*
    PlayerbotsPolicyCatalog::Find(
        std::uint8_t phase) const
    {
        auto const itr =
            _policies.find(phase);

        if (itr == _policies.end())
            return nullptr;

        return &itr->second;
    }

    std::vector<std::uint8_t>
    PlayerbotsPolicyCatalog::GetPhaseIds() const
    {
        std::vector<std::uint8_t> result;

        result.reserve(
            _policies.size());

        for (auto const& entry : _policies)
            result.push_back(entry.first);

        return result;
    }
}
