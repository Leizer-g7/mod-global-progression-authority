#include "PullPolicy.h"

#include "Runtime/ProgressionRuntime.h"

namespace GPA
{
    PullPolicySnapshot GetPullPolicySnapshot()
    {
        PullPolicyRuntimeView const view =
            sProgressionRuntime.GetPullPolicyRuntimeView();

        PullPolicySnapshot snapshot;

        snapshot.gmBypass =
            view.gmBypass;

        snapshot.status =
            ClassifyPullPolicy(
                view.configLoaded,
                view.enabled,
                view.configValid,
                view.activeDefinition.has_value(),
                view.authorityReady);

        if (snapshot.HasActivePolicy() &&
            view.activeDefinition)
        {
            snapshot.definition =
                *view.activeDefinition;
        }

        return snapshot;
    }
}
