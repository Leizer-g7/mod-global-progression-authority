#include "Adapters/AuctionHouseBotAdapter.h"
#include "Adapters/IndividualProgressionAdapter.h"

void AddSC_global_progression_authority();

void Addmod_global_progression_authorityScripts()
{
    GPA::Adapters::RegisterIndividualProgressionAdapter();
    GPA::Adapters::RegisterAuctionHouseBotAdapter();

    AddSC_global_progression_authority();
}
