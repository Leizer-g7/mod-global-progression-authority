#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_MGR_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_MGR_H

#include "PhaseCatalog.h"
#include "ProgressionState.h"
#include "ProgressionTransition.h"

#include <optional>
#include <string>

namespace GPA
{
    class ProgressionMgr
    {
    public:
        ProgressionMgr();

        bool Initialize(
            ProgressionState const& initialState,
            bool allowRollback,
            std::string& error);

        bool IsInitialized() const;

        bool IsRollbackAllowed() const;

        ProgressionState const* GetActiveState() const;

        PhaseDefinition const* GetActiveDefinition() const;

        TransitionResult EvaluateTransition(
            ProgressionState const& target) const;

        TransitionResult RequestTransition(
            ProgressionState const& target);

    private:
        PhaseCatalog _catalog;

        std::optional<ProgressionState> _activeState;

        bool _allowRollback = false;
    };
}

#endif
