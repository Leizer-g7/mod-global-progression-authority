#include "TravelPullPolicy.h"

#include "AreaDefines.h"
#include "Chat.h"
#include "Log.h"
#include "MapMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Transport.h"

#include <cstdint>

namespace
{
    constexpr std::uint32_t MAP_OUTLAND_ID = 530;
    constexpr std::uint32_t MAP_NORTHREND_ID = 571;

    /*
     * Naxxramas 533 is intentionally excluded from the global
     * Northrend travel gate.
     *
     * Grim/Individual Progression may reuse it for historical
     * Naxxramas content before WotLK.
     */
    constexpr std::uint32_t MAP_NAXXRAMAS_ID = 533;

    /*
     * TBC starter-zone areas not currently exposed by AreaDefines.h.
     */
    constexpr std::uint32_t AREA_VEILED_SEA_ID = 3479;
    constexpr std::uint32_t AREA_AMMEN_VALE_ID = 3526;


    bool IsTbcStarterZone(
        std::uint32_t zoneId)
    {
        switch (zoneId)
        {
            case AREA_EVERSONG_WOODS:
            case AREA_GHOSTLANDS:
            case AREA_SILVERMOON_CITY:
            case AREA_AZUREMYST_ISLE:
            case AREA_BLOODMYST_ISLE:
            case AREA_THE_EXODAR:
            case AREA_AMMEN_VALE_ID:
            case AREA_VEILED_SEA_ID:
                return true;

            default:
                return false;
        }
    }


    std::uint32_t ResolveExpansionMapId(
        std::uint32_t mapId)
    {
        /*
         * Explicit historical-content exception.
         */
        if (mapId == MAP_NAXXRAMAS_ID)
            return mapId;

        InstanceTemplate const* instanceTemplate =
            sObjectMgr->GetInstanceTemplate(mapId);

        if (!instanceTemplate)
            return mapId;

        if (instanceTemplate->Parent == MAP_OUTLAND_ID ||
            instanceTemplate->Parent == MAP_NORTHREND_ID)
        {
            return instanceTemplate->Parent;
        }

        return mapId;
    }


    GPA::TravelTarget ResolveTravelTarget(
        Player* player,
        std::uint32_t mapId,
        float x,
        float y,
        float z)
    {
        GPA::TravelTarget target;

        if (!player)
            return target;

        std::uint32_t const expansionMapId =
            ResolveExpansionMapId(mapId);

        if (expansionMapId == MAP_NORTHREND_ID)
        {
            target.expansion =
                GPA::TravelExpansion::Northrend;

            target.directContinent =
                mapId == MAP_NORTHREND_ID;

            return target;
        }

        if (expansionMapId == MAP_OUTLAND_ID)
        {
            target.expansion =
                GPA::TravelExpansion::Outland;

            target.directContinent =
                mapId == MAP_OUTLAND_ID;

            /*
             * Blood Elf / Draenei starter zones physically live on
             * map 530 but never belong to the global Outland gate.
             */
            if (target.directContinent)
            {
                std::uint32_t const zoneId =
                    sMapMgr->GetZoneId(
                        player->GetPhaseMask(),
                        mapId,
                        x,
                        y,
                        z);

                target.starterZone =
                    IsTbcStarterZone(zoneId);
            }

            return target;
        }

        return target;
    }


    GPA::TravelPullDecision GetTravelDecision(
        Player* player,
        std::uint32_t mapId,
        float x,
        float y,
        float z)
    {
        GPA::PullPolicySnapshot const snapshot =
            GPA::GetPullPolicySnapshot();

        GPA::TravelTarget const target =
            ResolveTravelTarget(
                player,
                mapId,
                x,
                y,
                z);

        return GPA::ClassifyTravelPullPolicy(
            snapshot,
            player && player->IsGameMaster(),
            target,
            player
                ? static_cast<std::uint8_t>(
                    player->GetLevel())
                : 0);
    }


    char const* GetRestrictionMessage(
        GPA::TravelRestriction restriction)
    {
        switch (restriction)
        {
            case GPA::TravelRestriction::OutlandPhase:
                return
                    "Terrallende aun no esta disponible "
                    "en la fase actual del servidor.";

            case GPA::TravelRestriction::OutlandLevel:
                return
                    "Necesitas ser al menos nivel 58 "
                    "para viajar a Terrallende.";

            case GPA::TravelRestriction::NorthrendPhase:
                return
                    "Rasganorte aun no esta disponible "
                    "en la fase actual del servidor.";

            case GPA::TravelRestriction::NorthrendLevel:
                return
                    "Necesitas ser al menos nivel 68 "
                    "para viajar a Rasganorte.";

            case GPA::TravelRestriction::AuthorityUnavailable:
                return
                    "El viaje a contenido de expansion esta "
                    "temporalmente bloqueado porque la autoridad "
                    "global de progresion no esta disponible.";

            case GPA::TravelRestriction::None:
            default:
                return "";
        }
    }


    void SendBlockedMessage(
        Player* player,
        char const* message)
    {
        if (!player ||
            !player->GetSession())
        {
            return;
        }

        ChatHandler handler(
            player->GetSession());

        handler.SendNotification(message);
    }


    void SafelyRemoveFromTransport(
        Player* player)
    {
        if (!player)
            return;

        Transport* transport =
            player->GetTransport();

        if (!transport)
            return;

        transport->RemovePassenger(
            player,
            true);

        player->RepopAtGraveyard();
    }


    bool BlockTravel(
        Player* player,
        GPA::TravelRestriction restriction)
    {
        /*
         * Preserve V1 transport safety behavior. A denied continent
         * transfer while attached to a transport must not leave the
         * player in a broken passenger state.
         */
        if (player &&
            player->GetTransport())
        {
            SafelyRemoveFromTransport(player);
        }

        SendBlockedMessage(
            player,
            GetRestrictionMessage(restriction));

        return false;
    }


    bool CanTravel(
        Player* player,
        std::uint32_t mapId,
        float x,
        float y,
        float z)
    {
        GPA::TravelPullDecision const decision =
            GetTravelDecision(
                player,
                mapId,
                x,
                y,
                z);

        if (!decision.IsBlocked())
            return true;

        return BlockTravel(
            player,
            decision.restriction);
    }


    bool EnsureValidLocation(
        Player* player)
    {
        if (!player)
            return true;

        GPA::TravelPullDecision const decision =
            GetTravelDecision(
                player,
                player->GetMapId(),
                player->GetPositionX(),
                player->GetPositionY(),
                player->GetPositionZ());

        if (!decision.IsBlocked())
            return true;

        /*
         * Use the real race/class starting location exactly as V1.
         *
         * This avoids hardcoding Stormwind/Orgrimmar and keeps the
         * Blood Elf / Draenei starter topology valid.
         */
        PlayerInfo const* startInfo =
            sObjectMgr->GetPlayerInfo(
                player->getRace(),
                player->getClass());

        if (!startInfo)
        {
            LOG_ERROR(
                "module",
                "GPA: travel login safety could not obtain "
                "PlayerInfo for {} (race={}, class={}).",
                player->GetName(),
                static_cast<unsigned>(
                    player->getRace()),
                static_cast<unsigned>(
                    player->getClass()));

            return false;
        }

        std::uint32_t const oldMap =
            player->GetMapId();

        bool const teleported =
            player->TeleportTo(
                startInfo->mapId,
                startInfo->positionX,
                startInfo->positionY,
                startInfo->positionZ,
                startInfo->orientation);

        if (!teleported)
        {
            LOG_ERROR(
                "module",
                "GPA: travel login safety could not relocate {} "
                "from map {}.",
                player->GetName(),
                oldMap);

            return false;
        }

        if (player->GetSession())
        {
            ChatHandler handler(
                player->GetSession());

            handler.SendNotification(
                "Tu ubicacion anterior no esta disponible "
                "en la fase actual. Has sido trasladado "
                "a una zona segura.");
        }

        LOG_INFO(
            "module",
            "GPA: travel login safety relocated {} "
            "from map {} to map {}. Reason={}.",
            player->GetName(),
            oldMap,
            startInfo->mapId,
            GetRestrictionMessage(
                decision.restriction));

        return true;
    }


    class GlobalProgressionTravelScript final
        : public PlayerScript
    {
    public:
        GlobalProgressionTravelScript()
            : PlayerScript(
                "GlobalProgressionTravelScript")
        {
        }

        void OnPlayerLogin(
            Player* player) override
        {
            EnsureValidLocation(player);
        }

        bool OnPlayerBeforeTeleport(
            Player* player,
            uint32 mapId,
            float x,
            float y,
            float z,
            float /*orientation*/,
            uint32 /*options*/,
            Unit* /*target*/) override
        {
            return CanTravel(
                player,
                mapId,
                x,
                y,
                z);
        }
    };
}


void AddSC_global_progression_travel_policy()
{
    new GlobalProgressionTravelScript();
}
