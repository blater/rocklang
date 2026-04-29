---
title: Comments and Includes
category: syntax
tags: [comments, include, modules]
sources: []
updated: 2026-04-11
status: current
---

# Comments and Includes

## Comments

The lexer skips two comment forms:

```rock
// single-line comment

/* block comment */
```

`//` comments run until the next newline. `/* ... */` comments run until the closing marker. A lone `/` is the division operator, not a comment marker.

## Includes

```rock
include "path/to/file.rkr"
```

The parser resolves the path with `get_abs_path()`, lexes the included file, and splices its tokens into the current token stream at the include site.

Included files must begin with a module declaration:

```rock
module Assert;
```

The include mechanism tracks files already included in the current parse session and skips duplicate includes. Include resolution is part of the parser layer; see [[parser-overview]] for token splicing detail and [[concepts/compilation-pipeline]] for where include handling fits in the compile flow.

## Practical Pattern

```rock
include "test/Assert.rkr"

sub main() {
  Assert a;
  a.AssertEQ("works", 1, 1);
}
```

See [[syntax/modules-and-records]] for module declarations and [[syntax/functions-and-methods]] for method calls.
