#include "alloc.h"
#include "fundefs.h"
#include "fundefs_internal.h"
#include "typedefs.h"

void printPass(string desc);

void printFail(string desc, string expected, string actual);

int printResult(int res, string desc, string expected, string actual);

int printIntResult(int res, string desc, int expected, int actual);

int AssertFalse(string desc, int value);

int AssertTrue(string desc, int value);

int AssertEQ(string desc, int expected, int actual);

int AssertNEQ(string desc, int expected, int actual);

int AssertEquals(string desc, string expected, string actual);

int AssertNotEquals(string desc, string expected, string actual);

void testRkrUnit(void);

int TRUE = 1;
int FALSE = 0;
void printPass(string desc)
{string __strtmp_0; __rock_make_string(&__strtmp_0, "PASS: ", 6);
printf("%s", __strtmp_0.data);
printf("%s", desc.data);
string __strtmp_1; __rock_make_string(&__strtmp_1, "\n", 1);
printf("%s", __strtmp_1.data);
}

void printFail(string desc, string expected, string actual)
{string __strtmp_2; __rock_make_string(&__strtmp_2, "FAIL: ", 6);
printf("%s", __strtmp_2.data);
printf("%s", desc.data);
string __strtmp_3; __rock_make_string(&__strtmp_3, " (expected:", 11);
printf("%s", __strtmp_3.data);
printf("%s", expected.data);
string __strtmp_4; __rock_make_string(&__strtmp_4, " actual:", 8);
printf("%s", __strtmp_4.data);
printf("%s", actual.data);
string __strtmp_5; __rock_make_string(&__strtmp_5, ")\n", 2);
printf("%s", __strtmp_5.data);
}

int printResult(int res, string desc, string expected, string actual)
{if (res == 1)
{printPass(desc);
}else
{printFail(desc, expected, actual);
}return res;
}

int printIntResult(int res, string desc, int expected, int actual)
{string __strtmp_6; __to_string_int(&__strtmp_6, expected);
string __strtmp_7; __to_string_int(&__strtmp_7, actual);
return printResult(res, desc, __strtmp_6, __strtmp_7);
}

int AssertFalse(string desc, int value)
{if (value == 1)
{string __strtmp_8; __rock_make_string(&__strtmp_8, "False(0)", 8);
string __strtmp_9; __to_string_int(&__strtmp_9, value);
printFail(desc, __strtmp_8, __strtmp_9);
return 0;
}else
{printPass(desc);
return 1;
}}

int AssertTrue(string desc, int value)
{if (value == 0)
{string __strtmp_10; __rock_make_string(&__strtmp_10, "TRUE(1)", 7);
string __strtmp_11; __to_string_int(&__strtmp_11, value);
printFail(desc, __strtmp_10, __strtmp_11);
return 0;
}else
{printPass(desc);
return 1;
}}

int AssertEQ(string desc, int expected, int actual)
{return printIntResult(expected == actual, desc, expected, actual);
}

int AssertNEQ(string desc, int expected, int actual)
{return printIntResult(expected != actual, desc, expected, actual);
}

int AssertEquals(string desc, string expected, string actual)
{return printResult(str_eq(expected, actual) != 0, desc, expected, actual);
}

int AssertNotEquals(string desc, string expected, string actual)
{return printResult(str_eq(expected, actual) == 0, desc, expected, actual);
}

void testRkrUnit(void)
{string __strtmp_12; __rock_make_string(&__strtmp_12, "should fail", 11);
AssertEQ(__strtmp_12, 1, 2);
string __strtmp_13; __rock_make_string(&__strtmp_13, "should pass", 11);
AssertEQ(__strtmp_13, 2, 2);
string __strtmp_14; __rock_make_string(&__strtmp_14, "should pass", 11);
string __strtmp_15; __rock_make_string(&__strtmp_15, "some-string", 11);
string __strtmp_16; __rock_make_string(&__strtmp_16, "some-string", 11);
AssertEquals(__strtmp_14, __strtmp_15, __strtmp_16);
string __strtmp_17; __rock_make_string(&__strtmp_17, "should fail", 11);
string __strtmp_18; __rock_make_string(&__strtmp_18, "some-string", 11);
string __strtmp_19; __rock_make_string(&__strtmp_19, "some-other-string", 17);
AssertEquals(__strtmp_17, __strtmp_18, __strtmp_19);
string __strtmp_20; __rock_make_string(&__strtmp_20, "should pass", 11);
string __strtmp_21; __rock_make_string(&__strtmp_21, "some-string", 11);
string __strtmp_22; __rock_make_string(&__strtmp_22, "some-other-string", 17);
AssertNotEquals(__strtmp_20, __strtmp_21, __strtmp_22);
}

int main(int argc, char **argv) {
init_compiler_stack();
fill_cmd_args(argc, argv);
{string __strtmp_23; __rock_make_string(&__strtmp_23, "hello", 5);
string x = __strtmp_23;
string __strtmp_24; __rock_make_string(&__strtmp_24, "test_simple", 11);
string __strtmp_25; __rock_make_string(&__strtmp_25, "hello", 5);
AssertEquals(__strtmp_24, __strtmp_25, x);
}kill_compiler_stack();
return 0;
}

