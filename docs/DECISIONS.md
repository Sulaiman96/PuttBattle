# PUTT BATTLE — Decision Log

Append-only record of design/architecture decisions and resolved conflicts.
Per CONVENTIONS §10: when a plan file and the code disagree with `README.md`,
README wins — record the conflict here with date and resolution. Also log any
deliberate deviation from a plan, and the rationale.

Foundational decisions (D1…D22) are described in `docs/README.md`; this file is
the running log of decisions made *during implementation*.

## Format

Each entry:

```
### D-<n> — <short title>   (YYYY-MM-DD)
**Context:** what forced a decision (conflict, ambiguity, trade-off).
**Decision:** what we chose.
**Why:** the reasoning, including options rejected.
**Touches:** files / plans / tags / channels affected.
```

---

<!-- New entries below, newest last. -->
