Wiki Schema - Rock language

## Purpose
This is the LLM's and architects internal knowledge base for the rock language transpiler and target architectures and compilers
The LLM writes and maintains this wiki. The architecture team and the LLM read it.  The architect directs it, and supplies sources. Do not wait for instructions to cross-reference - always cross-reference proactively.
**At the start of every session:** display the contents of "README.md" (which should contain basic instructions such as the workflows to orient the user, then check `new/` for pending sources and report how many are waiting (if any).
**When the user asks for help or how the wiki works:** display "README.md" and elaborate if needed

## Directory Layout
../src		- The rock transpiler source folder
../src/test	- The rock language unit test folder
wikiroot/	- This folder, root of the wiki documentation
index.md	- content catalog (LLM-maintained)
Log.md		- chronological ingest/query/lint log (LLM-maintained)
principles.md	- the wiki pattern document (reference only, do not modify)
new/		- drop zone for pending source documents (not yet ingested)
raw/		- ingested source documents (immutable - read, never modify or delete
pages/	
  overview.md	- high-level architecture synthesis (always keep current)
  ubiquitous-language.md - domain glossary and ubiquitous language (always keep current)
  domain-model.md	- bounded contexts, domain map, key relationships
  lexer/
  parser/
  generator/
  targets/
  testing/
  syntax/	- syntax summary index and per-keyword/concept pages with examples
  decisions/	- Architecture Decision Records (ADRs)
  concepts/	- lanuguage/parser/lexer/target domain concepts

### Source document lifecycle

Raw sources may be markdown documents, source code from /src or /test, sample code, or text files, or a file containing web/github links

**To add a source for ingestion:** drop the file into `new/`. No other step needed - its presence there is the signal that it is pending.

**After ingestion:** move the file from `new/` to `raw/`. Files in `raw/` are permanent and immutable - never edited, never deleted. Wiki pages link to files in `raw/`.

**Ingest state is therefore derived from the filesystem:** 
- `new/` contains anything = there are pending sources 
- `raw/` contains everything already ingested

**Re-ingestion:** if a revised version of a document arrives in new/ with the same filename as a file already in
raw/, do not proceed automatically - flag it to the user and agree how to handle it before moving anything

## Page Frontmatter

Every page in pages/ must include this YAML frontmatter:
```yaml
---
title: <human-readable title>
category: concepts | lexer | parser | generator | overview | targets | testing | syntax | decisions
tags: []
sources: [] # filenames from raw/ that this page draws from
updated: YYYY-MM-DD
status: current | draft | stale
---
```

- tags should use kebab-case terms from the ubiquitous language where possible. 
- `sources` lists raw filenames (not paths) e.g. '[requirements_v2.md]'.
- `status: stale` means the page likely needs updating from a newer source.


## Special Pages

### pages/overview.md"
High-Level synthesis of the entire transpiler and generation architecture. Updated on every ingest that touches system-level concerns. Should always be readable as a standalone orientation doc for a new architecture team member. Covers: platform topology, major domains, key architectural decisions, notable constraints.

### pages/ubiquitous-language.md
The canonical domain glossary. Every significant domain term encountered during ingest must be defined here. E ntries are alphabetically ordered. Format per entry:

```markdown
### <Term>
<Definition - precise, context-specific to this exchange, not generic>

**Domain:** <which bounded context this term primarily belongs to>
**See also:** [[<related terms>]]
```

When a term already exists and a new source uses it differently or adds nuance, update the definition and note the variation. This file is the source of truth for language. Never use a term in a wiki page without ensurin it is defined here.

### `pages/domain-model.md`
A living map of bounded contexts: what each context owns, its invariants, how it integrates with adjacent contexts. Updated when ingest reveals new domains or integration patterns. Use a Mermaid diagram to visualise context boundaries and relationships.


## Page Categories

### `concepts/`
Domain and technical concepts that need definition beyond the one-liner in ubiquitous-language.md. 

### `lexer/`
Architectural overview of lexer with key features and functions, describing technical flow

### `parser/`
Architectural overview of parser with key features and functions, describing technical flow

### `generator/`
One page per significant target - for now simply C, possibly sub pages for any sufficiently complex nuances for particular features e.g. modules

### `targets/`
Target platforms for the generated code - tooling required, tooling usage and conventions, special features and flags, any interfaces/adapter patterns in use, flags and any hardcodings/technical debt around the target support

### `testing/`
Details of testing approach, test harness use and examples.

### `syntax/` 
A syntax summary/index page, and sub pages for each keyword/concept together with example snippet to clarify

### `decisions/`
Architecture Decision Records. See ADR format below.

---

## ADR Format

File naming: decisions/ADR-NNNN-short-title.md (zero-padded, e.g. ADR-0001-...). markdown

```markdown
---
title: "ADR-NNNN: <Title›" 
category: decision 
tags: []
sources: []
updated: YYYY-MM-DD
status: proposed | accepted | deprecated / superseded 
superseded-by: ""  # ADR number if status is superseded
---

# ADR-NNNN: <Title>

**Status:** Accepted | Proposed | Deprecated | Superseded by ADR-XXXX
**Date:** YYYY-MM-DD

## Context
<What is the architectural problem or decision point? What forces are at play?>

## Decision
<What was decided?>

## Consequences
<What becomes easier or harder as a result? Known trade-offs, risks, debt.>

## Alternatives Considered
<What else was evaluated and why was it rejected?>

---

## Workflows

### Ingest

**Before ingesting:** check whether the file already exists in 'raw/ under the same name. If it does, stop an d flag this to the user as a potential re-ingest - do not proceed until the user confirms how to handle it.

When told to ingest a source file (or all pending sources):
1.  Glob 'new/ to identify files to ingest (skip .gitkeep').
2.  Read each source in full.
3.  Briefly discuss key architectural takeaways with the user before writing anything (unless asked to ingest silently).
4.  Write or update a **summary page** for the source in the most appropriate category. Name it after the sourc e with a-summary suffix using kebab-case (e.g.spot-margin-requirements-summary.md').
5.  Update pages/ubiquitous-language.md with any new or refined terms.
6.  Update "pages/domain-model.md' if new domains, boundaries, or integration patterns are revealed.
7.  Update pages/overview.md" if system-level concerns are materially affected.
8.  Update or create any relevant system, product, domain, or concept pages touched by the source.
9.  Update 'index.md" with any new pages.
10. Move the file from new/ to raw/
11. Append an entry to "log.md" recording the filename, pages created, and pages updated. single source will typically touch 5-15 pages. That is expected and correct. When asked to **ingest anything pending**: glob new/ - every file found there is pending. Ingest each one in turn.

### Refinement TODOs

During ingest, the LLM may encounter ambiguities, incorrect terminology, contradictions between sources, or areas needing clarification. These should **not block ingest**. 

Instead:
1. Note the issue inline on the relevant wiki page using this format:
> **TODO:** <description of the issue -what is ambiguous, contradictory, or incorrect, and which sources a re involved›
2. Append a TODO entry to "log.md" using the type todo': ## [YYYY-MM-DD] todo | <short title> ‹description of the issue and which pages/sources it affects>
4. Continue ingesting.

TODOs are reviewed later during a dedicated refinement pass or lint. The user may ask to review outstanding TO DOs at any time - the LLM should grep log.md for todo entries and grep wiki pages for > **TODO:*** blocks


### Query

When asked a question against the wiki:
1.  Read "index.md" to identify relevant pages.
2.  Read the relevant pages.
3.  Synthesise an answer with inline citations to page filenames.
4.  If the answer is non-trivial and reusable, ask the user if it should be filed as a new wiki page (a concept, analysis, or comparison page).


---

### Lint
When asked to lint the wiki:
**Content health:**
1.    Contradictions between pages.
2.    Claims likely superseded by newer sources (check sources frontmatter and log dates).
3.    Orphan pages - no inbound links from any other page.
4.    Important concepts mentioned but lacking their own page.
5.    Missing cross-references between obviously related pages.
6.    Pages with status: draft that are mature enough to promote.
7.    Stale pages - updated date significantly older than recent ingests on the same topic.

**Structural health:**
8. Overcrowded directories - any `pages/<category>/` with more than ~15 pages is a signal to consider sub-categories
(e.g. parser/errors/, parser/grammar/*).
9.  Miscategorised pages -pages that clearly belong in a different directory than where they sit.
10. Missing categories - a cluster of pages that share a cross-cutting concern not represented by any current directory (e.g. `modules/` emerging as its own domain). Propose new directories with a rationale.
11.  Redundant categories - directories with only 1-2 pages that would sit better elsewhere.
12.  "index.md" drift - entries that are missing, mis-described, or point to pages that no longer exist.
5.   Pending sources - glob new/ and report any files found there. These are pending ingest and should not be forgotten.

**Output:**
-   Produce a prioritised list of issues across both content and structure, grouped by type.
-   For any proposed structural change (new directory, rename, page move, describe the change and ask for appro val before executing - moving files has git history implications.
-   Suggest new questions to investigate or sources to seek out.
-   Ask user which issues to fix immediately vs. defer.

The folder structure is intentionally expected to evolve. Lint is the primary mechanism for proposing and agre eing structural changes.

---

## Cross-Referencing Conventions
- Link pages using [[filename-without-extension]] (Obsidian wiki-link style). 
- Every system page must link to its owning domain page.
- Every product page must link to the system pages it involves.
- Every concept that has its own page must be linked on first mention in any other page.
- Ubiquitous language terms should be bolded on first use in a page: **<term>**

---

## Index Format (index.md')
Organised by category. Each entry: - [[filename]] - one-line description'. Updated on every ingest.

---

## Log Format ("log.md")
Append-only. Each entry header: *## [YYYY-MM-DD] <type> | <title>
Types: "ingest", "query", "lint", "update", "todo".

Example:
```
## [2026-04-09] ingest 1 requirements_202602.md
Summary of what was learned and which pages were created or updated.
```


