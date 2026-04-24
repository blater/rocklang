#ifndef ROCKER_FUNDEFS
#define ROCKER_FUNDEFS

#include "typedefs.h"
#include "asm_interop.h"
#include <stdio.h>

void print(string s);
char *string_to_cstr(string s);
void cstr_to_string(string *out, char *cstr);
char get_nth_char(string s, int n);
void __concat_char(string *out, string s, char c);
void __concat_str(string *out, string s1, string s2);
int get_string_length(string s);
void set_nth_char(string s, int n, char c);
void __read_file_impl(string *out, string filename);
void new_string(string *out, string s);
void write_string_to_file(string s, string filename);
int str_eq(string s1, string s2);
void __get_abs_path_impl(string *out, string path);
void __free_string(string s);

// Configurable string index base (0 = zero-indexed, 1 = one-indexed)
extern int __rock_substr_index_base;
void set_string_index_base(int base);
int  get_string_index_base(void);
void __substring_from(string *out, string s, int start);
void __substring_range(string *out, string s, int start, int end);

// Helper for string construction (replaces C99 compound literals)
void __rock_make_string(string *out, const char *data, size_t length);

// byte/word/dword/float casting
int    __to_int_byte(byte b);
byte   __to_byte_int(int n);
void __to_string_byte(string *out, byte b);
int    __to_int_word(word w);
word   __to_word_int(int n);
void __to_string_word(string *out, word w);
int    __to_int_dword(dword d);
dword  __to_dword_int(int n);
void __to_string_dword(string *out, dword d);
void __to_string_int(string *out, int n);
int    __to_int_float(float f);
void __to_string_float(string *out, float f);

// Host-only wrappers for file I/O (returns by value)
#ifndef __SDCC

static inline string read_file(string filename) {
  string res;
  __read_file_impl(&res, filename);
  return res;
}

static inline string get_abs_path(string path) {
  string res;
  __get_abs_path_impl(&res, path);
  return res;
}

#else

// Stubs for ZXN (file I/O not available on ZXN)
static inline string read_file(string filename) {
  (void)filename;
  string res = {NULL, 0};
  return res;
}

static inline string get_abs_path(string path) {
  (void)path;
  string res = {NULL, 0};
  return res;
}

#endif

#ifndef __SDCC
#define to_int(x)    _Generic((x), byte: __to_int_byte, word: __to_int_word, dword: __to_int_dword, float: __to_int_float, default: (int)(x))(x)
#define to_byte(x)   _Generic((x), int:  __to_byte_int, default: (byte)(x))(x)
#define to_word(x)   _Generic((x), int:  __to_word_int, default: (word)(x))(x)
#define to_dword(x)  _Generic((x), int:  __to_dword_int, default: (dword)(x))(x)
#else
#define to_int(x)    ((int)(x))
#define to_byte(x)   ((byte)(x))
#define to_word(x)   ((word)(x))
#define to_dword(x)  ((dword)(x))
#endif
#define to_float(x)  ((float)(x))

#endif // ROCKER_FUNDEFS