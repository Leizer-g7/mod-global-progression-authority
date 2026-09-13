# GPA Architecture

## Core principle

Global Progression Authority is the sole owner of global progression state.

    Phase
    ContentStage

GPA decides policy.

External modules remain responsible for their own domain behavior.

## Responsibility boundary

GPA answers:

    WHEN is something allowed?

Domain modules answer:

    WHAT does that progression state mean?

Example:

    GPA:
        ContentStage = 4

    Individual Progression:
        Determines which historical content belongs to stage 4.

## Planned structure

    GPA Core
        ProgressionMgr
        ProgressionState
        PhaseDefinition
        validation
        persistence
        rollback

    Adapters
        PlayerAdapter
        ProfessionAdapter
        PlayerbotsAdapter
        IndividualProgressionAdapter
        AuctionHouseBotAdapter
        TravelAdapter
        LfgAdapter
        BattlegroundAdapter

    Commands
    Scripts
    Database

## Dependency rule

GPA Core must not depend directly on headers from:

- mod-playerbots
- mod-individual-progression
- mod-ah-bot-plus

Only the corresponding adapter may know the external module implementation.

## Optional integrations

GPA must continue operating when optional modules are absent.

A missing optional adapter must disable only that integration, not GPA Core.

## External modification policy

Preferred order:

1. Existing AzerothCore/module hook or public API.
2. GPA adapter using that API.
3. Small generic extension point in the external project.
4. Direct external modification only as a last resort.

No progression policy should be implemented inside an external module.

## State model

The authoritative state is:

    ActivePhase
    ActiveContentStage

Phase controls broad server capability.

ContentStage controls progression inside a level/endgame phase.

Example:

    Phase 60 / ContentStage 0
    Phase 60 / ContentStage 1
    ...
    Phase 60 / ContentStage 8

followed by:

    Phase 70 / ContentStage 8

## Rollback

Rollback is a first-class design requirement.

Adapters should apply reversible runtime policy whenever possible.

Persistent/destructive mutations must be avoided unless there is no viable
runtime alternative.
