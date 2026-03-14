| Statement/Function | Parameters | Return Value | Description |
|---|---|---|---|
| `!=` | left, right | bool | Not equal |
| `%` | left, right | numeric | Modulo (remainder) |
| `*` | left, right | numeric | Multiply |
| `+` | left, right | numeric | Add |
| `-` | left, right | numeric | Subtract |
| `/` | left, right | numeric | Divide (integer) |
| `<` | left, right | bool | Less than |
| `<=` | left, right | bool | Less than or equal |
| `=` | left, right | bool | Equal |
| `>` | left, right | bool | Greater than |
| `>=` | left, right | bool | Greater than or equal |
| `::` | record_variable (struct instance to access)<br>field_name (name of field within the record) | value (the contents of that field) | Accesses a named field within a record/struct and returns its current value |
| `:=` | target_variable (variable to assign to)<br>value_expression (expression whose result becomes the new value) | N/A | Assigns the result of an expression to a variable, replacing its previous value |
| `append()` | array (dynamic array to modify)<br>element (value to add to the array) | none | Adds a new element to the end of a dynamic array, growing it by one position |
| `byte` | (no parameters) | N/A | Declares a variable or parameter as a byte, an 8-bit unsigned integer ranging from 0 to 255 |
| `char` | (no parameters) | N/A | Declares a variable or parameter as a char, a single character that can be printed or manipulated |
| `concat()` | base_string (string to concatenate to)<br>additional (string or char to append to base_string) | string (new concatenated string) | Joins two strings together or appends a single character to a string, returning the combined result |
| `dim` | variable_name (identifier for the new variable)<br>type (what kind of data it holds)<br>expression (initial value to assign) | N/A | Declares a new variable with a type and assigns an initial value to it |
| `dword` | (no parameters) | N/A | Declares a variable or parameter as a dword, a 32-bit unsigned integer |
| `else` | statement (code to execute if preceding if condition is false) | N/A | Specifies code that runs only when the preceding if condition evaluates to false |
| `enum` | name (identifier for the enumeration)<br>members (comma-separated list of named constant values) | N/A | Defines an enumeration type with named constant values that can be used throughout the program |
| `for` | loop_variable (iterator variable that takes each value)<br>array (collection to iterate over) | N/A | Loops through each element in an array, binding each element to the loop variable in turn |
| `get()` | array (dynamic array to read from)<br>index (integer position of element to retrieve) | element (value stored at that position in the array) | Retrieves the element at a specific position in an array and returns its value |
| `if` | condition (boolean expression to evaluate) | N/A | Tests a boolean condition and executes the following code only if the condition is true |
| `include` | filepath (path to Rock source file to import, in quotes) | N/A | Loads and includes code from another Rock source file, making its definitions available in the current file |
| `int` | (no parameters) | N/A | Declares a variable or parameter as an int, a signed integer capable of holding positive and negative whole numbers |
| `length()` | string (string value or variable to measure) | int (length of the string in characters) | Returns the number of characters currently in a string |
| `let` | identifier (variable or function name)<br>type (data type for variable or return type for function)<br>expression (value for variable or body for function) | N/A | Declares a new variable with optional initialization, or declares a named function with parameters and body |
| `loop` | variable (iterator variable that holds current iteration value)<br>start (first number in the range)<br>end (last number in the range)<br>body (code to execute repeatedly) | N/A | Executes a block of code multiple times, with a loop variable counting from start to end |
| `NULL` | (no parameters) | pointer (null/empty pointer value) | Represents a null pointer or empty reference, used to indicate the absence of a valid memory address or object |
| `peek()` | address (word value representing a memory address to read from) | byte (the 8-bit unsigned integer value stored at that address) | Reads and returns a single byte value directly from a memory address |
| `poke()` | address (word value representing a memory address to write to)<br>value (byte value to store at that address) | none | Writes a single byte value directly to a specified memory address |
| `pop()` | array (dynamic array to remove from) | element (the value that was removed from the end of the array) | Removes and returns the last element from a dynamic array, shrinking it by one position |
| `print()` | string (message or variable containing text to output) | none | Outputs a string to standard output (stdout), typically the console or terminal |
| `printf()` | format_string (C-style format string with placeholders like %d, %s)<br>arguments (values to substitute into placeholders) | int (number of characters printed, or error code) | Outputs formatted text to standard output using C-style printf formatting, substituting values into the format string |
| `putchar()` | character (single char value to output) | none | Outputs a single character to standard output |
| `read_file()` | filepath (string path to the file to read) | string (contents of the file as a single string) | Reads an entire file from disk and returns its contents as a string; only available on HOST target |
| `record` | name (identifier for the record type)<br>fields (comma-separated field_name: type pairs defining the structure) | N/A | Defines a new record/struct type with named fields of specified types |
| `return` | value (expression whose result becomes the function's return value) | N/A | Exits the current function and optionally returns a value to the caller |
| `string` | (no parameters) | N/A | Declares a variable or parameter as a string, a sequence of characters with dynamic length |
| `sub` | name (function identifier)<br>parameters (formal parameters in parentheses)<br>return_type (type of value the function returns)<br>body (code that executes when function is called) | N/A | Declares a named function with parameters, return type, and implementation |
| `substring()` | string (string to extract from)<br>start_index | string (substring from start_index to end of original string) | Extracts a substring beginning at start_index and continuing to the end of the original string. Uses 1-based indexing |
| `substring()` | string (string to extract from)<br>start_index (1-indexed position where extraction begins)<br>end_index (1-indexed position where extraction ends) | string (substring from start_index to end_index) | Extracts a substring beginning at start_index and ending just before end_index. Uses 1-based indexing |
| `then` | statement (code to execute if preceding if condition is true) | N/A | Separates the if condition from the code that should execute when the condition is true |
| `to_byte()` | value (int, word, or dword to convert) | byte (the integer converted to 8-bit unsigned form) | Converts an integer, word, or dword to a byte by truncating to the lower 8 bits |
| `to_dword()` | value (int to convert) | dword (the integer promoted to 32-bit unsigned form) | Converts an int to a dword, extending it to a full 32-bit unsigned integer |
| `to_int()` | value (byte, word, or dword to convert) | int (the value converted to signed integer form) | Converts a byte, word, or dword to an int for use in arithmetic or assignments |
| `to_word()` | value (int to convert) | word (the integer converted to 16-bit unsigned form) | Converts an int to a word by truncating to the lower 16 bits |
| `toString()` | value (int, byte, word, or dword to convert) | string (decimal representation of the number as a string) | Converts a numeric value to its string representation in decimal, allowing it to be printed or concatenated |
| `word` | (no parameters) | N/A | Declares a variable or parameter as a word, a 16-bit unsigned integer |
| `write_string_to_file()` | string (text content to write)<br>filepath (string path to the file to write to) | none | Writes a string to a file on disk, creating or overwriting the file; only available on HOST target |

---

## Return Value Legend

- **N/A** — The concept of a return value does not apply (declarations, type definitions, and control flow statements)
- **none** — The statement/function executes but produces no returnable value
- **type** — Concrete type name (int, string, byte, etc.)

---

## Function Calls

When you call a function (declared with `let` or `sub`), the call expression returns the type specified in the function's return type annotation. For example:

```
let add(a: int, b: int): int => {
  return a + b;
}

let result: int => add(5, 3);  // Calling add() returns an int
```

Calling a function in an expression context returns its declared return type. This is different from the function declaration itself, which is a statement and returns N/A.
