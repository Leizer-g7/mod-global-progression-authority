#include "IndividualProgressionAdapter.h"

#include "../Runtime/ProgressionRuntime.h"

#include <cstdint>

#if __has_include("../../../mod-individual-progression/src/GlobalProgressionProvider.h")
#include "../../../mod-individual-progression/src/GlobalProgressionProvider.h"
#define GPA_HAS_INDIVIDUAL_PROGRESSION_BRIDGE 1

#if defined(__GNUC__) || defined(__clang__)
namespace IndividualProgressionGlobalAuthority
{
    void RegisterContentStageProvider(
        ContentStageProvider provider)
        __attribute__((weak));
}
#endif

#else
#define GPA_HAS_INDIVIDUAL_PROGRESSION_BRIDGE 0
#endif

namespace GPA::Adapters
{
    namespace
    {
#if GPA_HAS_INDIVIDUAL_PROGRESSION_BRIDGE
        bool ProvideContentStage(
            std::uint8_t& contentStage)
        {
            auto const activeState =
                sProgressionRuntime
                    .GetManager()
                    .GetActiveState();

            if (!activeState)
                return false;

            contentStage =
                activeState->contentStage;

            return true;
        }
#endif
    }

    void RegisterIndividualProgressionAdapter()
    {
#if GPA_HAS_INDIVIDUAL_PROGRESSION_BRIDGE
        auto registerProvider =
            &IndividualProgressionGlobalAuthority::
                RegisterContentStageProvider;

#if defined(__GNUC__) || defined(__clang__)
        if (!registerProvider)
            return;
#endif

        registerProvider(
            &ProvideContentStage);
#endif
    }
}
