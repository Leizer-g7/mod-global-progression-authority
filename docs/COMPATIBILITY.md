# GPA Compatibility Baseline

Baseline audit date:

2026-09-13

## Production baseline before GPA development

AzerothCore:
c2bc54d69a7c9b0de8cc5c4753544dbada7d3077

mod-phase-progression:
545f6c0833bd0685564e56d7bc436d221b069976

mod-individual-progression custom:
a691f5a004a64f4b007ebee5991b04d79d373129

Certified production worldserver image:
sha256:f394400e45707c9c41cc822ac397ff4ba4063dcfb8b29aae4206ef48debfcbb2

Production progression state:

    Phase 20
    ContentStage 0

## Upstream audit

AzerothCore local vs grimfeather/master:

    local-only commits: 4
    upstream commits ahead: 200

GPA-related local Core hooks are not present upstream:

    OnPlayerCanLearnTrainerSpell
    OnPlayerQuestComputeMaxLevelMoney

mod-individual-progression custom vs origin/master:

    custom-only commits: 12
    upstream commits ahead: 52

No external/global progression provider API was found upstream.

mod-playerbots local vs origin/master:

    local committed divergence: 0
    upstream commits ahead: 55

Required GPA configuration fields remain available upstream.

mod-ah-bot-plus local HEAD vs origin/master:

    committed divergence: 0
    upstream commits ahead: 0

No generic external item-eligibility or pricing callback suitable for GPA
was found in the audited upstream source.

## Compatibility policy

A module update is never deployed directly to production.

Process:

    fetch
    compare
    adapter/API compatibility review
    isolated build
    cloned-database smoke test
    production backup
    deploy certified image
