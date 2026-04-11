# Wiki Ingest Procedure

Process one source file at a time. Leave the wiki richer and more consistent after every ingest.

## Steps

### 1. Identify Sources

- If a specific file was given: use that file
- Otherwise: glob `wikiroot/new/` (exclude `.gitkeep`) — every file found is pending

### 2. Pre-flight Check (per file)

- Check whether the filename already exists in `wikiroot/raw/`
- **If it does: STOP.** Flag the collision to the user. Ask how to handle before proceeding. Do not move anything.
- If clear: proceed

### 3. Read and Discuss

- Read the source file in full
- Briefly summarise the key architectural takeaways for the user:
  - What is this source about?
  - What's new or surprising vs. the existing wiki?
  - Which pages will be most affected?
- (Skip discussion only if the user asked to ingest silently)

### 4. Write or Update Pages

Execute all of these for each source:

- [ ] **Summary page** — write a page in the most appropriate `pages/<category>/` directory named `<source-basename>-summary.md` (kebab-case). Add full frontmatter with `sources: [<filename>]`.
- [ ] **`pages/ubiquitous-language.md`** — add any new domain terms; refine existing definitions if the source adds nuance. Keep entries alphabetically ordered.
- [ ] **`pages/domain-model.md`** — update if new bounded contexts, integration patterns, or relationships are revealed. Update Mermaid diagram if structure changed.
- [ ] **`pages/overview.md`** — update if system-level concerns are materially affected (new targets, major architectural changes, key constraints).
- [ ] **Other relevant pages** — read and update any page whose content is touched by this source.
- [ ] **Cross-references** — add `[[filename]]` links between newly related pages. Bold domain terms on first use.
- [ ] **TODOs** — if the source reveals ambiguities, contradictions, or gaps: add `> **TODO:** <description>` inline on the relevant page(s).

### 5. Update Index

- Read `wikiroot/index.md`
- Add entries for any new pages created. Format: `- [[filename]] — one-line description`
- Entries are organised by category

### 6. Finalise the Ingested Source

- Move the file from `wikiroot/new/<filename>` to `wikiroot/raw/<filename>` only after the wiki page updates for that source have succeeded
- If the ingest is interrupted or fails before the page updates are complete, leave the source in `wikiroot/new/`
- Files in `raw/` are immutable — never edit or delete them after this point

### 7. Log and Commit

- Each successfully ingested source is one change batch unless the user explicitly approves a different batch boundary
- After the page updates and `new/` → `raw/` move are complete, run `./.agents/skills/wiki/scripts/log_and_commit.sh` with the exact repo-relative changed file list
- Include the moved source path in `wikiroot/new/` when it was tracked, the new file in `wikiroot/raw/`, every updated or created wiki page, `wikiroot/index.md` when changed, and any other touched wiki files
- Use a concise summary body that records what was learned and what changed

Use this pattern:
```
./.agents/skills/wiki/scripts/log_and_commit.sh ingest "<source filename>" \
  wikiroot/new/<filename> \
  wikiroot/raw/<filename> \
  wikiroot/pages/<category>/<page>.md \
  wikiroot/pages/overview.md \
  wikiroot/index.md <<'EOF'
Summary: <2-4 sentence summary of what was learned>
Pages created: [list]
Pages updated: [list]
TODOs filed: [count and brief description, or "none"]
EOF
```

If any TODOs were filed, append a `todo` entry for each in the same batch before running the helper so the ingest commit captures the full state.

### 8. Repeat

If multiple files were pending, process the next one. Each file gets its own full pass through steps 2–7 and normally its own commit.

---

## Notes

- The source code in `src/` is the source of truth. If a source document conflicts with the code, note the conflict as a TODO — do not let the wiki claim something the code contradicts.
- When in doubt about which category a page belongs in, use `concepts/` as a safe default and flag for lint review.
- `raw/` sources may be markdown, source code, test files, or text. Read them all the same way.
