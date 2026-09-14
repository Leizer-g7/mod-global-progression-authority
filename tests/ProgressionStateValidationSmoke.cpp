#include "ProgressionMgr.h"

#include <iostream>
#include <string>

namespace
{
    bool Check(
        GPA::ProgressionMgr const& manager,
        GPA::ProgressionState const& state,
        bool expected)
    {
        std::string error;

        bool const valid =
            manager.ValidateState(state, error);

        if (valid != expected)
            return false;

        if (valid && !error.empty())
            return false;

        if (!valid && error.empty())
            return false;

        return true;
    }
}

int main()
{
    GPA::ProgressionMgr manager;

    if (manager.IsInitialized())
        return 1;

    if (!Check(manager, {20, 0}, true))
        return 2;

    if (!Check(manager, {60, 8}, true))
        return 3;

    if (!Check(manager, {70, 8}, true))
        return 4;

    if (!Check(manager, {80, 18}, true))
        return 5;

    if (!Check(manager, {20, 1}, false))
        return 6;

    if (!Check(manager, {60, 9}, false))
        return 7;

    if (!Check(manager, {70, 7}, false))
        return 8;

    if (!Check(manager, {80, 19}, false))
        return 9;

    if (manager.IsInitialized())
        return 10;

    if (manager.GetActiveState().has_value())
        return 11;

    if (manager.GetActiveDefinition() != nullptr)
        return 12;

    std::cout
        << "STATE_VALIDATION_VALID_MATRIX=OK\n"
        << "STATE_VALIDATION_INVALID_MATRIX=OK\n"
        << "STATE_VALIDATION_NON_DESTRUCTIVE=OK\n";

    return 0;
}
