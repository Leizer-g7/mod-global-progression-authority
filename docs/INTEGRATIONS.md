# GPA Integration Classification

Classification:

A = functionality belongs inside GPA
B = GPA should consume an existing public API or hook
C = functionality belongs outside GPA
D = minimal external extension point currently required

## GPA Core

Phase / ContentStage state
Classification: A

Validation
Classification: A

Persistence
Classification: A

Rollback
Classification: A

## Players

Player level cap
Classification: A/B

Quest XP-to-money behavior at GPA level cap
Classification: D

Current requirement:
generic AzerothCore PlayerScript hook for max-level quest money computation.

## Professions

Profession skill caps
Classification: A/B

Trainer spell/rank filtering
Classification: D

Current requirement:
generic AzerothCore PlayerScript hook determining whether a trainer spell
may be learned.

## Playerbots

Random bot maximum level
Classification: B

Level brackets/distribution
Classification: B

Gear limits
Classification: B

Death Knight availability
Classification: B

Travel destinations
Classification: B

LFG
Classification: B

Battleground automation
Classification: B

Playerbots progression integration currently requires no GPA-specific patch.

The existing WPVP randombot lease API is unrelated to GPA.

## Individual Progression

Historical content rules
Classification: C

Global ContentStage authority
Classification: D -> target B

Target architecture:

    IPP
      -> generic external progression provider
      -> GPA IndividualProgressionAdapter

IPP should retain its original behavior when no external authority exists.

## AH Bot Plus

Normal AHBot behavior
Classification: C

Phase-aware item eligibility
Classification: D -> target B

Target extension:

    Can AHBot list this ItemTemplate?

GPA should provide the progression policy.

Rarity pricing
Classification: C

Source-rarity pricing
Classification: C

Duplicate listing limits
Classification: C

Those features are AH economy enhancements and must not be part of GPA Core.

## Core changes unrelated to GPA

Alterac Valley creature/boss fix
Classification: C

Playerbots data copy into Docker runtime image
Classification: C
