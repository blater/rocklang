# DRY Analysis: C Bootstrapper Code Refactoring Opportunities

## Summary
Found ~15 significant DRY violations across 2,690 LOC that create maintenance burden and make type system refactoring harder.

## Detailed Opportunities

| # | Location | Issue | Impact | Refactoring Suggestion | Difficulty |
|---|----------|-------|--------|----------------------|------------|
| 1 | `src/generator.c:61-111` `generate_append()` | Highly similar to `generate_get()`, `generate_set()`, `generate_pop()` (4 nearly identical functions) | **High** - Any type system change requires 4x edits | Extract to `generate_array_op(op_type, call, g)` function that takes operation suffix (`_push_array`, `_get_elem`, etc.) | Medium |
| 2 | `src/generator.c:70-104` (all 4 array functions) | Repeated pattern: identifier vs funcall type checking in all 4 functions | **High** - Type inference logic duplicated | Extract `get_array_element_type(arr, g->table)` helper function | Low |
| 3 | `src/generator.c:70-82` `get_ref()` + type lookup | Same pattern repeated in append, get, set, pop: lookup variable → check if vardef → extract type | **High** - Will be needed by proper type system | Extract `get_array_var_type(identifier, table)` helper | Low |
| 4 | `src/token.c:34-58` `lexeme_of_type()` & `type_of_lexeme()` | Assert statements check `TOK_COUNT == 45` (now 46) - must update in multiple places | **Medium** - Error-prone, breaks with token additions | Replace asserts with compile-time check or remove redundancy | Low |
| 5 | `src/parser.c:62-83` `is_assign()` function | Manually checks token sequence for assignment (checks for TOK_BIG_ARROW) | **Medium** - Similar logic might exist elsewhere | Create `is_statement_terminator(token_type)` and `scan_for_operator(parser, op_type)` helpers | Low |
| 6 | `src/parser.c:202-230` `parse_statement()` | Long if-else chain: `if (a == TOK_IF)`, `if (a == TOK_WHILE)`, etc. (10+ branches) | **High** - Hard to add new statement types, unclear dispatch | Consider jump table or handler map: `{ TOK_IF, parse_if }, { TOK_WHILE, parse_while }, ...` | Medium |
| 7 | `src/generator.c:454-540` `generate_expression()` | Long if-else chain checking `expr->tag`: `if == literal`, `else if == identifier`, etc. | **High** - Same as parse_statement - hard to extend | Extract to handler map/jump table | Medium |
| 8 | `src/generator.c:629-668` `generate_statement()` | Long if-else chain checking `stmt->tag` (9+ branches) | **High** - Parallel to generate_expression | Extract to handler map/jump table | Medium |
| 9 | `src/generator.c:340-398` Array helper generation (`string_push_array()`, `string_get_elem()`, etc.) | Code generation for type-specific array helpers is manually written per type | **High** - Blocks proper polymorphic array support | Create single `generate_array_helpers(type_name, f)` template function | Medium |
| 10 | `src/token.c:6-21` lexemes array | Token lexeme strings hardcoded and indexed by enum - fragile | **Medium** - Error-prone during token additions | Add static assertion that array length == TOK_COUNT | Low |
| 11 | `src/generator.c:61-111` Type name extraction | All 4 array functions extract `type_name` the same way from identifier | **Medium** - Duplicated code | Fold into `get_array_var_type()` helper | Low |
| 12 | `src/ast.h:20-45` typedef forward declarations | Multiple `typedef struct X X;` patterns | **Low** - Unavoidable in C, but creates verbosity | Consider macro: `#define AST_NODE(name) typedef struct name name;` | Low |
| 13 | `src/generator.c` | Error messages inconsistently formatted across functions | **Low** - Minor maintenance issue | Standardize error message prefix/format | Low |
| 14 | `src/name_table.c` & `src/parser.c` | Scope management code duplicated in `new_nt_scope()`, `end_nt_scope()`, plus parser scope handling | **Medium** - Type system will need enhanced scope tracking | Extract scope management to separate module | Medium |
| 15 | `src/main.c:100-101` & `src/generator.c:817-819` | Hardcoded paths to `src/generation/*` in two places | **Low** - Minor duplication | Could be constants or config | Low |

---

## Refactoring Priority Tiers

### Tier 1: Foundation (Enable Type System) - **LOW effort, HIGH impact**
These enable both Option 1 (incremental) and Option 4 (proper type system).

- **#2**: Extract `get_array_element_type()` - ~30 lines
- **#3**: Extract `get_array_var_type()` - ~20 lines
- **#4**: Fix token count assertions - ~5 lines
- **Total: ~50 LOC, enables all other work**

### Tier 2: Type System Foundation - **MEDIUM effort, CRITICAL impact**
Required for Option 4; helpful for Option 1.

- **#1**: Consolidate array operation functions → `generate_array_op()` - ~80 lines saved
- **#9**: Extract `generate_array_helpers()` template - ~40 lines saved
- **#14**: Extract scope management to `scope.h/c` - ~60 lines

### Tier 3: Dispatch Cleanup - **MEDIUM effort, HIGH maintainability**
Makes code clearer and easier to extend.

- **#6**: `parse_statement()` → handler map - ~50 lines
- **#7**: `generate_expression()` → handler map - ~80 lines
- **#8**: `generate_statement()` → handler map - ~50 lines

### Tier 4: Polish - **LOW effort, LOW impact**
Nice-to-haves that improve code quality.

- **#5**: Parser helpers
- **#10**: Token assertions
- **#12**: AST typedefs macro
- **#13**: Error message standardization
- **#15**: Path constants

---

## Recommended Path Forward

### If pursuing **Option 1** (incremental fixes):
1. Do **Tier 1** refactoring (~1-2 hours)
2. Implement targeted type inference for specific expression types as needed
3. Minimal scope expansion

### If pursuing **Option 4** (proper type system):
1. Do **Tier 1** refactoring first
2. Do **Tier 2** refactoring to set up proper type infrastructure
3. Build comprehensive type evaluation system on the extracted helpers
4. Do **Tier 3** to make new handler maps work with type system
5. Result: Clean, maintainable, extensible compiler

---

## Code Quality Metrics

| Metric | Current | After Tier 1 | After Tier 1+2 | After All |
|--------|---------|--------------|-----------------|-----------|
| DRY Violations | 15 | 7 | 3 | 0-1 |
| Max Function Length | ~200 LOC | ~200 LOC | ~150 LOC | ~100 LOC |
| Duplicated Logic | High | Medium | Low | Very Low |
| Extensibility | Poor | Fair | Good | Excellent |
