# mod-global-progression-authority

Global Progression Authority (GPA) for AzerothCore.

GPA is designed to be the single global progression authority for the server.

It owns the global progression state:

- Phase
- ContentStage

Other systems consume that state through isolated adapters.

Primary integrations:

- Players
- Professions
- Playerbots
- mod-individual-progression
- mod-ah-bot-plus
- LFG / RDF
- Battlegrounds
- Travel and expansion availability

## Design goal

Keep custom progression logic inside GPA instead of spreading modifications
through AzerothCore and external modules.

External patches must be avoided whenever possible. When unavoidable, they
must be small, generic extension points rather than GPA-specific logic.

## Production

The existing mod-phase-progression remains the production authority while
GPA is under development.

GPA must reach functional parity and pass isolated validation before it can
replace the production module.
