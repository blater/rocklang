# Wiki Lint Procedure

`/wiki lint` is report-only by default. `/wiki lint fix <scope>` applies an approved batch of fixes, then logs and commits that batch.

## Steps

### 1. Inventory

- Glob all files under `wikiroot/pages/` recursively
- Read `wikiroot/index.md`
- Glob `wikiroot/new/` (pending sources)
- Grep `wikiroot/log.md` for `todo` entries (pattern: `^## \[.*\] todo`)
- Grep all pages for inline `> **TODO:**` blocks

### 2. Content Health Checks

For each page under `pages/`:

- [ ] **Contradictions** — does any claim conflict with another page or with `src/`? (spot-check by topic; focus on pages sharing the same `tags` or `sources`)
- [ ] **Stale** — `updated` date significantly older than recent ingests on the same topic (`status: stale` or not yet marked stale)
- [ ] **Drafts ready to promote** — `status: draft` pages that look complete enough for `status: current`
- [ ] **Orphans** — no inbound `[[filename]]` links from any other page (grep all pages for the filename)
- [ ] **Missing cross-references** — pages that clearly relate but don't link to each other
- [ ] **Undefined terms** — bold terms (**term**) used in pages but absent from `ubiquitous-language.md`
- [ ] **Missing concept pages** — important concepts mentioned across multiple pages but lacking a dedicated page
- [ ] **Outstanding TODOs** — collect all `> **TODO:**` blocks and log.md `todo` entries found in step 1

### 3. Structural Health Checks

- [ ] **Overcrowded directories** — any `pages/<category>/` with more than ~15 pages → propose sub-categories
- [ ] **Miscategorised pages** — pages clearly in the wrong directory
- [ ] **Missing categories** — a cluster of related pages with no dedicated directory
- [ ] **Redundant categories** — directories with only 1–2 pages that would sit better elsewhere
- [ ] **Index drift** — `index.md` entries pointing to non-existent files; pages that exist but are missing from the index
- [ ] **Pending sources** — report any files found in `wikiroot/new/` (excluding `.gitkeep`)

### 4. Output

Produce a prioritised issue list grouped by type:

```
## Content Issues
P1: [Critical — contradictions, wrong claims]
P2: [Important — stale, orphans, undefined terms]
P3: [Nice-to-have — missing cross-refs, draft promotion]

## Structural Issues
[Proposed changes with rationale]

## Outstanding TODOs
[List from log.md and inline TODO blocks]

## Pending Sources
[Files in new/ awaiting ingest]

## Suggested next steps
[New questions to investigate, sources to seek]
```

### 5. Before Executing Structural Changes

- Describe the proposed change (move, rename, new directory) and its rationale
- **Ask for approval before executing** — moving files has git history implications
- For content fixes (updating frontmatter, adding cross-refs, correcting terms): offer to fix immediately

### 6. Applying an Approved Fix Batch

- Do not change files during plain `/wiki lint`; report only
- Apply fixes only when the user explicitly asks for a change batch such as `/wiki lint fix <scope>`
- Keep each batch focused and small enough to describe clearly in one log entry and one commit
- After updating the approved files, run `./.agents/skills/wiki/scripts/log_and_commit.sh` with the exact changed file list and a short stdin summary
- Include in the log body: which issues were fixed, what remains open, and any follow-up TODOs created

### 7. Log and Commit Example

Use this pattern for mutating lint runs:
```
./.agents/skills/wiki/scripts/log_and_commit.sh lint "<brief summary>" \
  wikiroot/pages/<page>.md \
  wikiroot/index.md <<'EOF'
Issues fixed: <what changed>
Remaining issues: <what was deferred, or "none">
TODOs filed: <count and short description, or "none">
EOF
```

The helper appends the `wikiroot/log.md` entry and creates one commit for that fix batch.
