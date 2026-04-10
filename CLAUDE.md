# Rock Project — LLM Standing Instructions

## Wiki

This project has a persistent knowledge wiki at `wikiroot/`. It is the compiled architectural understanding of the transpiler — always prefer it over guessing or re-deriving from scratch.

### Session start
At the start of every session, run the `/wiki` skill (no arguments). This displays the wiki README and reports any pending sources in `wikiroot/new/`.

### Building context for any task
Before reading source files, run the `/wiki query <topic>` skill first to orient. Then use `src/` to verify details or fill gaps. Applies to: adding features, fixing bugs, refactoring, tracing data flow — any task requiring system understanding.

### After significant code changes
Note which wiki pages are likely stale and mention this to the user. Do not silently let the wiki drift from the code.

### Ingest
When the user drops a file in `wikiroot/new/` or asks you to process pending sources, run the `/wiki ingest` skill.

### Lint
When the user asks to health-check or audit the wiki, run the `/wiki lint` skill.

---

## Reference discipline 

Use reference material from wikiroot/pages before guessing or inferring unfamiliar language syntax/semantics or technologies. 
If reference material is insufficient, ask for more, specifying the topic and what you are trying to solve.
