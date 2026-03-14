# Rock Language Usage Guide

This guide shows how to declare and use each type, and demonstrates every built-in function with code samples.

---

## Primitive Types

### int

```rock
dim x: int := 42;
dim y: int := -10;
dim z: int := 0;
```

### char

```rock
dim ch: char := 'A';
dim newline: char := '\n';
dim space: char := ' ';
```

### string

```rock
dim greeting: string := "Hello, World!";
dim empty: string := "";
dim message: string := "Line 1\nLine 2\n";
```

### boolean

Rock uses integers for boolean values (0 = false, non-zero = true):

```rock
dim flag: int := 1;      // true
dim condition: int := 0; // false

if flag then {
  // executed if flag != 0
}
```

---

## Composite Types

### Record (Struct)

**Declaration:**
```rock
record Point {
  x: int
  y: int
}

record Person {
  name: string
  age: int
  email: string
}
```

**Creation:**
```rock
dim p: Point := {x := 10, y := 20};
dim alice: Person := {
  name := "Alice",
  age := 30,
  email := "alice@example.com"
};
```

**Field Access:**
```rock
dim x_value: int := p.x;         // dot notation
dim name: string := alice::name; // colon-colon notation
```

**Field Update:**
```rock
p.x := 15;
alice::age := 31;
```

**Records with Array Fields:**
```rock
record Inventory {
  items: int[]
  quantities: int[]
}

dim inv: Inventory := {
  items := [],
  quantities := []
};
```

### Enum

**Declaration:**
```rock
enum Color {
  Red,
  Green,
  Blue
}

enum Status {
  Active,
  Inactive,
  Pending
}
```

**Usage:**
```rock
dim color: int := Red;

if color = Blue then {
  print("It is blue\n");
}
```

Enums are represented as integers (0, 1, 2, ...).

### Arrays

**Dynamic Arrays (grow as needed):**
```rock
dim numbers: int[] := [];
append(numbers, 10);
append(numbers, 20);
append(numbers, 30);
```

**Fixed-Size Arrays (declare with size):**
```rock
dim people: Person[2] := [];
```

**Array Access:**
```rock
dim arr: int[] := [];
append(arr, 100);
dim first: int := get(arr, 0);
print_int(first);  // prints: 100
```

**Array Update:**
```rock
dim arr: int[] := [];
append(arr, 5);
set(arr, 0, 99);   // arr[0] is now 99
```

**Array Iteration:**
```rock
include "lib/stdlib.rkr"

sub main() {
  dim nums: int[] := [];
  append(nums, 10);
  append(nums, 20);
  append(nums, 30);

  // Using iter loop
  iter num: nums {
    print_int(num);
    print(" ");
  }
  print("\n");
}
```

**Array of Records:**
```rock
record Item {
  id: int
  name: string
}

sub main() {
  dim items: Item[3] := [];
  dim item1: Item := {id := 1, name := "First"};
  append(items, item1);
}
```

---

## Built-in Functions

### I/O Functions

#### print(s: string)
Print a string:
```rock
print("Hello, World!\n");
print("The answer is ");
```

#### putchar(c: char)
Print a single character:
```rock
putchar('A');
putchar('\n');
```

#### exit(code: int)
Exit the program:
```rock
exit(0);  // success
exit(1);  // error
```

### String Functions

#### concat(s: string, x)
Concatenate string with another string or character:
```rock
dim s: string := "Hello";
dim result: string := concat(s, " World");  // string + string
dim result2: string := concat(result, '!'); // string + char
print(result2);  // prints: Hello World!
```

#### string_of_int(n: int): string
Convert integer to string:
```rock
dim num: int := 42;
dim text: string := string_of_int(num);
print(text);  // prints: 42
```

#### toString(n: int): string
Modern alias for string_of_int:
```rock
dim num: int := 123;
dim text: string := toString(num);
print(text);  // prints: 123
```

#### get_nth_char(s: string, n: int): char
Get character at index:
```rock
dim str: string := "hello";
dim ch: char := get_nth_char(str, 0);
putchar(ch);  // prints: h
```

#### set_nth_char(s: string, n: int, c: char)
Set character at index (modifies string in place):
```rock
dim str: string := "hello";
set_nth_char(str, 0, 'H');
print(str);  // prints: Hello
```

#### get_string_length(s: string): int
Get string length:
```rock
dim str: string := "hello";
dim len: int := get_string_length(str);
print_int(len);  // prints: 5
```

#### str_eq(s1: string, s2: string): int
Compare strings (returns 1 if equal, 0 if not):
```rock
dim a: string := "hello";
dim b: string := "hello";
dim c: string := "world";

if str_eq(a, b) then {
  print("Strings match\n");
}

if str_eq(a, c) then {
  print("Match\n");
} else {
  print("No match\n");  // executes
}
```

### Array Functions

#### append(arr: type[], elem: type)
Add element to end of array:
```rock
dim arr: int[] := [];
append(arr, 10);
append(arr, 20);
append(arr, 30);
```

#### get(arr: type[], index: int): type
Get element at index:
```rock
dim arr: int[] := [];
append(arr, 100);
dim first: int := get(arr, 0);
```

#### set(arr: type[], index: int, value: type)
Set element at index:
```rock
dim arr: int[] := [];
append(arr, 0);
set(arr, 0, 99);  // arr[0] is now 99
```

#### pop(arr: type[]): type
Remove and return last element:
```rock
dim arr: int[] := [];
append(arr, 1);
append(arr, 2);
append(arr, 3);
dim last: int := pop(arr);  // last = 3, arr now [1, 2]
```

#### length(arr: type[]): int
Get array length:
```rock
dim arr: int[] := [];
append(arr, 10);
append(arr, 20);
dim len: int := length(arr);  // len = 2
```

### File Functions

#### read_file(filename: string): string
Read entire file as string:
```rock
dim content: string := read_file("data.txt");
print(content);
```

#### write_string_to_file(s: string, filename: string)
Write string to file:
```rock
dim text: string := "Hello, File!\n";
write_string_to_file(text, "output.txt");
```

#### get_abs_path(path: string): string
Get absolute path:
```rock
dim relative: string := "./file.txt";
dim absolute: string := get_abs_path(relative);
print(absolute);
```

### Standard Library Functions

Include `lib/stdlib.rkr` to use these:

#### print_int(n: int)
Print integer to stdout:
```rock
include "lib/stdlib.rkr"

sub main() {
  print_int(42);
  print("\n");
}
```

#### create_string(src: string, offset: int, length: int): string
Extract substring:
```rock
dim str: string := "Hello, World!";
dim substr: string := create_string(str, 0, 5);
print(substr);  // prints: Hello
```

#### cons_str(src: string, offset: int): string
Get substring from offset to end:
```rock
dim str: string := "Hello, World!";
dim tail: string := cons_str(str, 7);
print(tail);  // prints: World!
```

---

## Control Flow

### If/Then/Else

```rock
dim x: int := 10;

if x > 5 then {
  print("x is greater than 5\n");
}

if x = 10 then {
  print("x equals 10\n");
} else {
  print("x is not 10\n");
}

if x < 0 then {
  print("negative\n");
} else if x = 0 then {
  print("zero\n");
} else {
  print("positive\n");
}
```

### While Loop

```rock
dim i: int := 0;
while i < 5 do {
  print_int(i);
  print(" ");
  i := i + 1;
}
print("\n");
```

### Loop (Numeric Range)

```rock
// Loop from 0 to 9
loop i: 0 -> 9 {
  print_int(i);
  print(" ");
}
print("\n");

// Optional => arrow (both work)
loop j: 0 -> 5 => {
  print_int(j);
}
```

### Iter (Array Iteration)

```rock
include "lib/stdlib.rkr"

sub main() {
  dim nums: int[] := [];
  append(nums, 10);
  append(nums, 20);
  append(nums, 30);

  iter num: nums {
    print_int(num);
    print(" ");
  }
  print("\n");
}
```

---

## Functions

### Function Declaration

**Basic function:**
```rock
sub add(a: int, b: int): int {
  return a + b;
}
```

**Function with no parameters:**
```rock
sub get_pi(): int {
  return 314;
}
```

**Function returning void:**
```rock
sub greet(name: string): void {
  print("Hello, ");
  print(name);
  print("!\n");
}
```

**Recursive function:**
```rock
sub factorial(n: int): int {
  if n <= 1 then return 1;
  return n * factorial(n - 1);
}
```

**Main function:**
```rock
sub main() {
  print("Program starts here\n");
}
```

---

## Complete Example

```rock
include "lib/stdlib.rkr"

record Point {
  x: int
  y: int
}

sub distance_from_origin(p: Point): int {
  dim x2: int := p.x * p.x;
  dim y2: int := p.y * p.y;
  return x2 + y2;
}

sub main() {
  // Variables
  dim count: int := 0;
  dim message: string := "Points: ";

  // Arrays
  dim points: Point[] := [];

  // Create records
  dim p1: Point := {x := 3, y := 4};
  dim p2: Point := {x := 5, y := 12};

  append(points, p1);
  append(points, p2);

  // Iterate and process
  iter pt: points {
    dim dist: int := distance_from_origin(pt);
    message := concat(message, toString(dist));
    message := concat(message, " ");
  }

  // Output
  print(message);
  print("\n");
}
```

---

**Last Updated**: 2026-03-05
