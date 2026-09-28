#ifndef MOD_GLOBAL_PROGRESSION_AUTHORITY_PLAYERBOTS_PHASE_ADAPTER_H
#define MOD_GLOBAL_PROGRESSION_AUTHORITY_PLAYERBOTS_PHASE_ADAPTER_H

namespace GPA
{
    class ICriticalRuntimeParticipant;

    /*
     * Returns the GPA critical runtime participant responsible for
     * applying Playerbots phase policy.
     *
     * Playerbots headers deliberately remain confined to the
     * implementation translation unit.
     */
    ICriticalRuntimeParticipant* GetPlayerbotsPhaseAdapter();
}

#endif
