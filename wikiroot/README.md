# Rock Language Wiki — README

This is the LLM-maintained knowledge base for the Rock language transpiler, its runtime library, and its target platforms.

## Orientation

Rock is a statically-typed language that transpiles to C. The compiler is written in C and supports two output targets: **host** (compiled with gcc) and **ZXN** (ZX Spectrum Next, compiled with Z88DK/SDCC for Z80).

The wiki lives at `wikiroot/pages/`. Start with [[overview]] for the big picture, then drill into component pages as needed.

## Workflows

### Session start
1. Read this file (you're doing it).
2. Glob `new/` — report any pending sources waiting for ingest.
3. Orient yourself with `index.md` if needed.

### Adding a source
Drop the file into `new/`. Tell the LLM to ingest it. The LLM reads, discusses key takeaways, updates relevant pages, moves the file to `raw/`, and appends to `log.md`.

### Querying
Ask questions naturally. The LLM reads `index.md`, drills into relevant pages, and synthesises an answer with citations.

### Ingesting pending sources
Say "ingest pending sources" or "ingest everything in new/". The LLM follows the ingest workflow from `wiki-proposal.md`.

### Linting
Say "lint the wiki". The LLM checks for contradictions, orphans, stale pages, structural issues, and proposes fixes.

## Key files

| File | Purpose |
|------|---------|
| `index.md` | Content catalog — read first when answering queries |
| `log.md` | Append-only chronological record of ingests, queries, lints |
| `wiki-proposal.md` | Schema, conventions, workflows (reference) |
| `principles.md` | Background on the LLM wiki pattern (reference) |
| `pages/overview.md` | High-level architecture — good starting point |
| `pages/ubiquitous-language.md` | Canonical glossary |
| `pages/domain-model.md` | Bounded context map with Mermaid diagram |
| `raw/` | Immutable ingested sources |
| `new/` | Drop zone for pending sources |

## Source layout (outside wikiroot)

```
../src/          Rock compiler source (C)
../src/lib/      Runtime library (fundefs_internal.c, headers)
../test/         Rock language test suite (.rkr files)
../docs/         Language specification and syntax reference
```
