# Wiki Query Procedure

Execute these steps in order for every query.

## Steps

1. **Read the index**
   - Read `wikiroot/index.md`
   - Identify which pages are relevant to the question

2. **Read relevant pages**
   - Read each relevant page in full
   - If a page links to another page that looks relevant, read that too
   - If the index has no matches, grep `wikiroot/pages/` for key terms from the question

3. **Synthesise**
   - Answer the question directly
   - Cite sources inline using `[[filename]]` links
   - If the answer required resolving a conflict between pages, note the conflict and which page you judged more authoritative (prefer the page with a more recent `updated` date and sources that postdate the other)

4. **Verify against source**
   - If the answer involves code-level details (tokens, AST nodes, builtins, etc.), cross-check against the relevant `src/` file before finalising
   - If the wiki and the source code disagree, trust the source code and note the discrepancy

5. **Offer to file the answer**
   - If the answer is non-trivial (more than a one-liner) and would be reusable, ask: *"Should I file this as a wiki page?"*
   - If yes: write it to the most appropriate `pages/<category>/` directory, add frontmatter, update `index.md`, append a `query` entry to `log.md`

## Notes

- Do not guess — if relevant pages don't exist, say so and suggest what source or code to read to answer the question
- Keep citations accurate: only cite pages you actually read
