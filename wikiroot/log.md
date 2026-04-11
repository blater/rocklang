# Rock Wiki — Log

Append-only chronological record of ingests, queries, lints, and updates.

---

## [2026-04-09] ingest | Bootstrap from src/ and test/

**Sources:** Direct bootstrap from `../src/` and `../test/` (not via new/ lifecycle — initial wiki creation).

**Files read:** `src/lexer.c`, `src/token.c`, `src/token.h`, `src/parser.c`, `src/ast.c`, `src/ast.h`, `src/generator.c`, `src/generator.h`, `src/name_table.c`, `src/main.c`, `src/lib/fundefs_internal.c`, `src/stringview.h`, `src/alloc.h`, `docs/SYNTAX.md`, all `test/*.rkr` files.

**Pages created (20):**

- `README.md` — wiki orientation and workflow guide
- `index.md` — content catalog
- `pages/overview.md` — high-level architecture synthesis
- `pages/ubiquitous-language.md` — domain glossary (28 terms)
- `pages/domain-model.md` — bounded context map with Mermaid diagram
- `pages/lexer/lexer-overview.md` — lexer internals
- `pages/parser/parser-overview.md` — parser internals and grammar
- `pages/generator/generator-overview.md` — code generation internals
- `pages/targets/host-gcc.md` — host target
- `pages/targets/zxn-z80.md` — ZXN/Z80 target
- `pages/testing/testing-overview.md` — test suite (27 tests)
- `pages/syntax/syntax-index.md` — keyword + builtin reference
- `pages/syntax/types.md` — type system and variable declarations
- `pages/syntax/control-flow.md` — if, while, for, match, operators
- `pages/syntax/functions-and-methods.md` — sub, methods, mangling
- `pages/syntax/arrays.md` — array operations
- `pages/syntax/strings.md` — string operations
- `pages/syntax/modules-and-records.md` — record, pro, enum, module
- `pages/syntax/embed.md` — @embed blocks
- `pages/concepts/compilation-pipeline.md` — pipeline step-by-step
- `pages/concepts/name-table.md` — symbol table design
- `pages/concepts/string-representation.md` — rock_string and temporaries
- `pages/concepts/array-internals.md` — __internal_dynamic_array_t

**Pages updated:**

- `wiki-proposal.md` — fixed incorrect `syntax/` category description (copy-paste artefact from unrelated project)

**Key architectural takeaways:**
- Three-phase pipeline: lex → parse → generate (transpile to C, not compile to machine code)
- Two targets: host (gcc, absolute includes) and ZXN (Z88DK/SDCC, relative includes, statement splitting)
- No semantic analysis phase — type inference and error detection happen inside the generator
- Arena allocator spans all phases; no incremental freeing
- Type-specific array wrappers synthesised by generator at each use site
- pre_f buffer pattern handles C statement/expression ordering constraints

---

## [2026-04-09] update | Fix rock/rockc/run_tests.sh documentation
Pages updated: `testing-overview.md`, `host-gcc.md`, `overview.md`.
Corrections: removed false `--target=zxn` flag from run_tests.sh docs (script is gcc-only); added single-file test mode. Clarified `rock` (full build script) vs `rockc` (transpiler binary) distinction. Fixed default output name (`<basename>.exe` not `out`). Added `--debug` flag documentation.

---

## [2026-04-10] lint | Link validity check
Issues found: 1 content, 0 structural, 2 TODOs.
- **Fixed:** `index.md` line 3 — `[[filename]]` format-example was a false wiki-link; replaced with plain text.
- **Links:** 35 unique `[[links]]` across all pages; 34/34 resolve correctly. 0 orphans. Index perfectly matches pages (34/34).
- **Images:** 46 image references across ZXN pages; all 46 resolve correctly to `raw/images/`.
- **TODOs outstanding:** (1) nested sub unimplemented in `functions-and-methods.md`; (2) `pro` type match semantics in `modules-and-records.md`.
- **Pending sources:** none in `new/`.

---

## [2026-04-10] ingest | zxnext_guide.md — ZX Spectrum Next hardware reference (Chapter 3)

**Source:** `raw/zxnext_guide.md` (10,177 lines; Tomaz Kragelj, 2022). Chapter 3 only — Chapter 2 (Z80 CPU) deferred.
**Images:** 195 images moved to `raw/images/` (referenced from pages as `../../../raw/images/`).

**Pages created (13):**

- `pages/targets/zxn-hardware.md` — ZXN hardware overview: layer stack, port access patterns, implementer notes
- `pages/targets/zxn/zxn-ports-registers.md` — All I/O ports + full Next register index with subsystem cross-links
- `pages/targets/zxn/zxn-memory-paging.md` — 8K/16K banking, MMU slots `$50`–`$57`, legacy 128K/+3 modes
- `pages/targets/zxn/zxn-dma.md` — zxnDMA: WR0–WR6, prescalar, examples, interrupt interaction
- `pages/targets/zxn/zxn-palette.md` — 8-bit/9-bit colour, palette editing, Layer 2 priority flag
- `pages/targets/zxn/zxn-ula.md` — Pixel/attribute memory, border, shadow screen, PIXELAD/PIXELDN/SETAE
- `pages/targets/zxn/zxn-layer2.md` — 256×192/320×256/640×256 modes, bank paging, double buffering
- `pages/targets/zxn/zxn-tilemap.md` — Tile definitions, 2-byte/1-byte tilemap data, stencil mode
- `pages/targets/zxn/zxn-sprites.md` — Pattern upload, 8/4-bit colour, anchor/composite/unified groups
- `pages/targets/zxn/zxn-copper.md` — WAIT/MOVE/HALT/NOOP, 2KB program memory, upload via `$63`
- `pages/targets/zxn/zxn-sound.md` — AY-3-8912 ×3 (Turbo Sound), registers 0–13, chip select
- `pages/targets/zxn/zxn-keyboard.md` — 8×5 matrix via `$xxFE`, extended keys `$B0`/`$B1`
- `pages/targets/zxn/zxn-interrupts.md` — IM1/IM2/Hardware IM2, 14-source vector table, line interrupt

**Pages updated:**
- `pages/targets/zxn-z80.md` — added ZXN Hardware Reference section linking all subsystem pages
- `pages/ubiquitous-language.md` — added 13 new terms: AY-3-8912, Bank, Copper, DMA, Layer 2, MMU Slot, Next Register, NEXTREG, Palette, Palette Offset, Sprite, Tilemap, ULA
- `pages/domain-model.md` — added ZXN Hardware bounded context + Mermaid nodes; added ZXN Hardware Context description
- `pages/overview.md` — added ZXN hardware reference mention in ZXN target section
- `index.md` — added hub page + 12 subsystem pages under new "ZXN Hardware Subsystems" section

**Key takeaways:**
- Next registers accessed via `NEXTREG` (20–24 T-states) — preferred over `$243B`/`$253B` ports (52–58 T-states)
- Four compositable graphics layers: ULA (base), Layer 2 (framebuffer), Tilemap (tiles), Sprites (top). Priority via `$15`.
- Layer 2 requires MMU bank swapping (8K at a time via slot 6) — too large for Z80 address space
- Sprite patterns stored in FPGA RAM — DMA via port `$xx5B` is the efficient load mechanism
- Copper co-processor eliminates per-scanline ISR overhead for raster effects
- DMA with prescalar enables sampled audio streaming at fixed sample rates
- Hardware IM2 provides 14 independently vectored interrupt sources on a 32-byte-aligned table

---

## [2026-04-09] todo | pro type match semantics unclear

**Issue:** The exact match semantics for `pro` (product type) values — specifically how match arms access the constructor payload — need clarification from source and tests. The `modules-and-records.md` page notes this with a TODO block.

**Pages affected:** `pages/syntax/modules-and-records.md`

**Resolution:** Ingest `test/` files that use `pro` types, or ask the architect to clarify.

## [2026-04-10] lint | fix zxn target docs and nested-sub reference

Changed files: 3

Issues fixed: corrected the ZXN target page to reflect the current rock/rockc/zcc workflow, include strategy, and runtime source paths; corrected the nested-sub TODO to point at generator.c.
Remaining issues: frontmatter/source attribution drift remains on overview.md and domain-model.md; pro match semantics TODO remains unresolved.
Link check: validated all [[...]] targets under wikiroot/pages; 34/34 page targets resolve to matching .md files.
TODOs filed: none


## [2026-04-10] lint | fix parser overview include resolution notes

Changed files: 2

Issues fixed: corrected parser_t field descriptions, updated include resolution to say paths are resolved relative to the including file, and corrected the nested-subs note to reference generator.c.
Remaining issues: none in the touched page from this pass.
TODOs filed: none


## [2026-04-10] lint | refresh syntax docs after declaration and array-field changes

Changed files: 15

Issues fixed: removed stale let/dim syntax from language pages, updated parser docs for type-first declarations and generic postfix chains, documented record-field arrays and expression receivers, corrected module examples and ZXN header-resolution notes, and refreshed testing docs to match the current harness and top-level suite.
Remaining issues: one intentional parser note still mentions removed let/dim syntax only to state that they are no longer accepted; broader wiki lint coverage outside syntax/build topics remains for a later pass.
TODOs filed: none


## [2026-04-10] lint | remove stale include-path and pipeline misconceptions

Changed files: 6

Issues fixed: removed stale claims that host builds emit absolute runtime include paths; updated pipeline examples to current new_parser(prog) and new_generator(cout) usage; aligned host/ZXN target docs with the driver-managed -I "$ROCK_ROOT/src/lib" include strategy.
Remaining issues: none found for the targeted misconception scan.
TODOs filed: none.


## [2026-04-10] lint | neutralise wiki review-tone and fix host target toolchain docs

Changed files: 10

Issues fixed: corrected the host target toolchain table so rock and rockc roles match the code; removed defensive wording about copied headers and old absolute-include behaviour; removed review-oriented phrasing such as "pre-existing, not a regression"; simplified syntax and overview pages to state current behaviour directly.
Remaining issues: none found in the targeted neutral-tone sweep across the touched pages.
TODOs filed: none.


## [2026-04-11] ingest | ZXN interrupt samples

Changed files: 16

Summary: Ingested the IM1, legacy IM2, Hardware IM2, and bus-safe IM2 assembly samples. Added a ZXN sample-program hub, an interrupt topic overview, and per-source summaries that explain setup order, memory placement, vector-table shape, and runtime implications.
Pages created: zxn-sample-programs, zxn-interrupt-samples, zxn-im1-sample-summary, zxn-im2-sample-summary, zxn-im2hw-sample-summary, zxn-im2safe-sample-summary.
Pages updated: zxn-interrupts, domain-model, overview, ubiquitous-language, index.
Review/lint: verified touched wiki links and raw source links; fixed a pre-existing glossary link typo while updating glossary terms.
TODOs filed: none.

