# zcc (z88dk) Compiler Notes

## CRITICAL: -I flag syntax
zcc requires NO SPACE between -I and the path argument.

- CORRECT: `-I${RT}/lib` or `-I/some/path`
- WRONG: `-I /some/path` (space between -I and path)

This is different from gcc which accepts both forms. Previous sessions incorrectly concluded that zcc's -I flag was broken - it was just the space syntax issue.
