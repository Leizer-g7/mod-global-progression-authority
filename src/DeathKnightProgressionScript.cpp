#include "DeathKnightPullPolicy.h"
#include "PullPolicy.h"

#include "ScriptMgr.h"
#include "SharedDefines.h"

namespace
{
    class GlobalProgressionDeathKnightAccountScript final
        : public AccountScript
    {
    public:
        GlobalProgressionDeathKnightAccountScript()
            : AccountScript(
                "GlobalProgressionDeathKnightAccountScript")
        {
        }

        bool CanAccountCreateCharacter(
            uint32 /*accountId*/,
            uint8 /*charRace*/,
            uint8 charClass) override
        {
            bool const isDeathKnight =
                charClass == CLASS_DEATH_KNIGHT;

            GPA::PullPolicySnapshot const snapshot =
                GPA::GetPullPolicySnapshot();

            return
                GPA::ClassifyDeathKnightCreationPolicy(
                    snapshot,
                    isDeathKnight) ==
                GPA::DeathKnightCreationDecision::Allow;
        }
    };
}

void AddSC_global_progression_death_knight_policy()
{
    new GlobalProgressionDeathKnightAccountScript();
}
