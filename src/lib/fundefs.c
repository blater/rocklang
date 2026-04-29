#include "fundefs.h"
#include "alloc.h"
#include "fundefs_internal.h"
#include "pools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// SIMPLE UTILITIES (safe for sccz80)
// ============================================================================

char *string_to_cstr(string s) { return s.data; }

void __rock_make_string(string *out, const char *data, size_t length) {
  out->data = (char *)data;
  out->length = length;
  out->capacity = 0;       /* read-only view by default */
  out->backing = NULL;     /* no refcounted backing (Phase E will populate) */
  out->owned = 0;
}

char charAt(string s, int n) {
  if (s.data == NULL || n >= s.length)
    return 0;
  return s.data[n];
}

int equals(string s1, string s2) {
  if (s1.data == NULL && s2.data == NULL)
    return 1;
  if (s1.data == NULL || s2.data == NULL)
    return 0;
  if (s1.length != s2.length)
    return 0;
  return memcmp(s1.data, s2.data, s1.length) == 0;
}

// ============================================================================
// STRING OPERATIONS (available everywhere with out-param convention)
// ============================================================================

void print(string s) {
  if (s.data == NULL) {
    printf("NULL");
    fflush(stdout);
    return;
  }
  for (int i = 0; i < s.length; i++)
    putchar(s.data[i]);
  fflush(stdout);
}

void cstr_to_string(string *out, char *cstr) {
  __rock_make_string(out, cstr, strlen(cstr));
}

void __concat_char(string *out, string s, char c) {
  if (s.data == NULL) {
    char *tmp = allocate_compiler_persistent(2);
    tmp[0] = c;
    tmp[1] = 0;
    __rock_make_string(out, tmp, 1);
    out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
    return;
  }
  char *tmp = allocate_compiler_persistent(s.length + 2);
  memcpy(tmp, s.data, s.length);
  tmp[s.length] = c;
  tmp[s.length + 1] = 0;
  __rock_make_string(out, tmp, s.length + 1);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

void __concat_str(string *out, string s1, string s2) {
  size_t len1 = (s1.data == NULL) ? 0 : s1.length;
  size_t len2 = (s2.data == NULL) ? 0 : s2.length;
  char *buffer = allocate_compiler_persistent(len1 + len2 + 1);
  if (s1.data != NULL)
    memcpy(buffer, s1.data, len1);
  if (s2.data != NULL)
    memcpy(&buffer[len1], s2.data, len2);
  buffer[len1 + len2] = 0;
  __rock_make_string(out, buffer, len1 + len2);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

void new_string(string *out, string s) {
  out->data = allocate_compiler_persistent(s.length + 1);
  out->length = s.length;
  for (int i = 0; i < out->length; i++)
    out->data[i] = s.data[i];
  out->data[out->length] = 0;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
  out->backing = NULL;          /* refcount machinery wired in Phase E */
  out->owned = 1;
}

void setCharAt(string s, int n, char c) {
  if (s.length > n)
    s.data[n] = c;
}

// ============================================================================
// FILE I/O (host-only, requires POSIX APIs)
// ============================================================================

#ifdef __SDCC
void __read_file_impl(string *out, string filename) {
  (void)filename;
  __rock_make_string(out, "", 0);
}

void write_string_to_file(string s, string filename) {
  (void)s;
  (void)filename;
}

void __get_abs_path_impl(string *out, string path) {
  (void)path;
  __rock_make_string(out, "", 0);
}
#else
void __read_file_impl(string *out, string filename) {
  FILE *f = fopen(string_to_cstr(filename), "r");
  if (f == NULL) {
    printf("Unable to open file \"%s\" for reading: ",
           string_to_cstr(filename));
    perror("");
    exit_rocker(1);
  }
  fseek(f, 0, SEEK_END);
  size_t length = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buffer = allocate_compiler_persistent(length + 1);
  fread(buffer, 1, length, f);
  buffer[length] = 0;
  fclose(f);
  __rock_make_string(out, buffer, length);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

void write_string_to_file(string s, string filename) {
  char *fname = string_to_cstr(filename);
  FILE *f = fopen(fname, "wb");
  if (f == NULL) {
    printf("Unable to open file \"%s\" for writing: ",
           string_to_cstr(filename));
    perror("");
    exit_rocker(1);
  }
  fprintf(f, "%s", s.data);
  fclose(f);
}

void __get_abs_path_impl(string *out, string path) {
  char *abs_path_tmp = realpath(path.data, NULL);
  if (abs_path_tmp == NULL) {
    printf("Could not get absolute path of file \'%s\'\n", path.data);
    exit(1);
  }
  size_t abs_len = strlen(abs_path_tmp);
  char *abs_path = allocate_compiler_persistent(abs_len + 1);
  memcpy(abs_path, abs_path_tmp, abs_len + 1);
  free(abs_path_tmp);

  string tmp;
  __rock_make_string(&tmp, abs_path, abs_len);
  new_string(out, tmp);
}
#endif

void __free_string(string *s) {
  if (s->data != NULL && s->owned) {
    deregister_compiler_persistent(s->data);
    free(s->data);
    s->data = NULL;
    s->length = 0;
    s->capacity = 0;
    s->backing = NULL;
    s->owned = 0;
  }
}

/* ADR-0003 §7.6: retain/release on string backing. See header for the
 * three-class discriminant. Until Phase H populates `backing` for
 * concat/toString/clone/read_file results and for string literals, every
 * descriptor in the wild has `backing == NULL` and these helpers no-op
 * unconditionally. The C-level unit tests in test/pools_test.c synthesise
 * fake backings to exercise the live paths. */

void __string_retain(string s) {
  if (s.backing == NULL) return;
  if (s.backing->refcount == ROCK_RC_STATIC) return;
  if (s.backing->refcount == ROCK_RC_FREE) {
    fprintf(stderr, "rock: __string_retain on freed block\n");
    exit(1);
  }
  s.backing->refcount++;
}

void __string_release(string s) {
  if (s.backing == NULL) return;
  if (s.backing->refcount == ROCK_RC_STATIC) return;
  if (s.backing->refcount == ROCK_RC_FREE) {
    fprintf(stderr, "rock: __string_release on already-freed block\n");
    exit(1);
  }
  if (--s.backing->refcount == 0) {
    /* Payload pointer is just past the header. */
    void *payload = (void *)((char *)s.backing + sizeof(rock_block_header));
    rock_longlived_free(payload);
  }
}

// ============================================================================
// STRING SLICING
// ============================================================================

int __rock_substr_index_base = 1;

void set_string_index_base(int base) { __rock_substr_index_base = base; }
int  get_string_index_base(void)     { return __rock_substr_index_base; }

static int __normalize_substr_idx(int idx, int length) {
  if (idx < 0) return length + idx;
  return idx - __rock_substr_index_base;
}

void __substring_from(string *out, string s, int start) {
  int c_start = __normalize_substr_idx(start, (int)s.length);
  if (c_start < 0 || c_start > (int)s.length) {
    printf("substring: start index out of bounds (start=%d, length=%d)\n",
           start, (int)s.length);
    exit_rocker(1);
  }
  int len = (int)s.length - c_start;
  char *buf = allocate_compiler_persistent(len + 1);
  memcpy(buf, s.data + c_start, len);
  buf[len] = 0;
  __rock_make_string(out, buf, (size_t)len);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

void __substring_range(string *out, string s, int start, int end) {
  int c_start = __normalize_substr_idx(start, (int)s.length);
  int c_end   = __normalize_substr_idx(end,   (int)s.length);
  if (c_start < 0 || c_start >= (int)s.length) {
    printf("substring: start index out of bounds (start=%d, length=%d)\n",
           start, (int)s.length);
    exit_rocker(1);
  }
  if (c_end < c_start || c_end >= (int)s.length) {
    printf("substring: end index out of bounds (end=%d, length=%d)\n",
           end, (int)s.length);
    exit_rocker(1);
  }
  int len = c_end - c_start + 1;
  char *buf = allocate_compiler_persistent(len + 1);
  memcpy(buf, s.data + c_start, len);
  buf[len] = 0;
  __rock_make_string(out, buf, (size_t)len);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

// ============================================================================
// TYPE CONVERSIONS
// ============================================================================

int __to_int_byte(byte b) {
  return (int)b;
}

byte __to_byte_int(int n) {
  return (byte)n;
}

void __to_string_byte(string *out, byte b) {
  char buf[4];  // max "255\0"
  int len = snprintf(buf, sizeof(buf), "%u", (unsigned int)b);
  char *out_buf = allocate_compiler_persistent(len + 1);
  memcpy(out_buf, buf, len + 1);
  __rock_make_string(out, out_buf, (size_t)len);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

void __to_string_int(string *out, int n) {
  char buf[24];
  int len = snprintf(buf, sizeof(buf), "%d", n);
  char *out_buf = allocate_compiler_persistent(len + 1);
  memcpy(out_buf, buf, len + 1);
  __rock_make_string(out, out_buf, (size_t)len);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

int  __to_int_word(word w)  { return (int)w; }
word __to_word_int(int n)   { return (word)n; }

void __to_string_word(string *out, word w) {
  char buf[6];  // max "65535\0"
  int len = snprintf(buf, sizeof(buf), "%u", (unsigned int)w);
  char *out_buf = allocate_compiler_persistent(len + 1);
  memcpy(out_buf, buf, len + 1);
  __rock_make_string(out, out_buf, (size_t)len);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

int   __to_int_dword(dword d)  { return (int)d; }
dword __to_dword_int(int n)    { return (dword)n; }
int   __to_int_float(float f)  { return (int)f; }

void __to_string_dword(string *out, dword d) {
  char buf[11];  // max "4294967295\0"
  int len = snprintf(buf, sizeof(buf), "%lu", (unsigned long)d);
  char *out_buf = allocate_compiler_persistent(len + 1);
  memcpy(out_buf, buf, len + 1);
  __rock_make_string(out, out_buf, (size_t)len);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

void __to_string_float(string *out, float f) {
  char buf[24];
  int len = snprintf(buf, sizeof(buf), "%g", (double)f);
  char *out_buf = allocate_compiler_persistent(len + 1);
  memcpy(out_buf, buf, len + 1);
  __rock_make_string(out, out_buf, (size_t)len);
  out->owned = 1;
  out->capacity = out->length;  /* writable backing; ADR §7.1 transitional */
}

// ============================================================================
// ALWAYS-AVAILABLE CASTING FUNCTIONS (safe for sccz80)
// ============================================================================

byte __to_byte_word(word w) { return (byte)w; }
word __to_word_byte(byte b) { return (word)b; }
