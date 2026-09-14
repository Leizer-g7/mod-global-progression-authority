#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_MGR_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PROGRESSION_MGR_H

#include "PhaseCatalog.h"
#include "ProgressionState.h"
#include "ProgressionTransition.h"

#include <mutex>
#include <optional>
#include <string>

namespace GPA
{
    class ProgressionBootstrap;
    class ProgressionTransitionCoordinator;

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

        bool ValidateState(
            ProgressionState const& state,
            std::string& error) const;

        std::optional<ProgressionState> GetActiveState() const;

        PhaseDefinition const* GetActiveDefinition() const;

        TransitionResult EvaluateTransition(
            ProgressionState const& target) const;

    private:
        friend class ProgressionBootstrap;
        friend class ProgressionTransitionCoordinator;

        bool IsInitializedUnlocked() const;

        bool InitializeUnlocked(
            ProgressionState const& initialState,
            bool allowRollback,
            std::string& error);

        TransitionResult EvaluateTransitionUnlocked(
            ProgressionState const& target) const;

        void CommitEvaluatedTransitionUnlocked(
            TransitionResult const& evaluation);

        PhaseCatalog _catalog;

        mutable std::mutex _transitionMutex;

        std::optional<ProgressionState> _activeState;

        bool _allowRollback = false;
    };
}

#endif
