#include "alloc.h"
#include "fundefs.h"
#include "fundefs_internal.h"
#include "typedefs.h"

typedef struct file *file;
typedef enum file_mode file_mode;
typedef enum term_color term_color;
typedef struct Person *Person;
typedef struct Team *Team;
file open_file(string filename, file_mode mode);

void close_file(file f);

string get_file_contents(file f);

void pfile(file f, string s);

void print_color(string src, term_color col);

void print_underline(string s);

void print_info();

void print_cmd();

void print_error();

void implemented(string s);

void compiler_assert(boolean b, string s);

void print_int(int n);

string string_of_int(int n);

string toString(int n);

string create_string(string src, int offset, int length);

string cons_str(string src, int offset);

__internal_dynamic_array_t file_make_array(void) {
  return __internal_make_array(sizeof(file), 0);
}

void file_push_array(__internal_dynamic_array_t arr, file elem) {
  __internal_push_array(arr, &elem);
}

file file_pop_array(__internal_dynamic_array_t arr) {
  file *res = __internal_pop_array(arr);
  return *res;
}

file file_get_elem(__internal_dynamic_array_t arr, size_t index) {
  file *res = __internal_get_elem(arr, index);
  if (res == NULL){ printf("NULL ELEMENT IN file_get_elem"); exit(1);}
  return *res;
}

void file_set_elem(__internal_dynamic_array_t arr, size_t index, file elem) {
  __internal_set_elem(arr, index, &elem);
}

void file_insert(__internal_dynamic_array_t arr, size_t index, file elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t Person_make_array(void) {
  return __internal_make_array(sizeof(Person), 0);
}

void Person_push_array(__internal_dynamic_array_t arr, Person elem) {
  __internal_push_array(arr, &elem);
}

Person Person_pop_array(__internal_dynamic_array_t arr) {
  Person *res = __internal_pop_array(arr);
  return *res;
}

Person Person_get_elem(__internal_dynamic_array_t arr, size_t index) {
  Person *res = __internal_get_elem(arr, index);
  if (res == NULL){ printf("NULL ELEMENT IN Person_get_elem"); exit(1);}
  return *res;
}

void Person_set_elem(__internal_dynamic_array_t arr, size_t index, Person elem) {
  __internal_set_elem(arr, index, &elem);
}

void Person_insert(__internal_dynamic_array_t arr, size_t index, Person elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t Team_make_array(void) {
  return __internal_make_array(sizeof(Team), 0);
}

void Team_push_array(__internal_dynamic_array_t arr, Team elem) {
  __internal_push_array(arr, &elem);
}

Team Team_pop_array(__internal_dynamic_array_t arr) {
  Team *res = __internal_pop_array(arr);
  return *res;
}

Team Team_get_elem(__internal_dynamic_array_t arr, size_t index) {
  Team *res = __internal_get_elem(arr, index);
  if (res == NULL){ printf("NULL ELEMENT IN Team_get_elem"); exit(1);}
  return *res;
}

void Team_set_elem(__internal_dynamic_array_t arr, size_t index, Team elem) {
  __internal_set_elem(arr, index, &elem);
}

void Team_insert(__internal_dynamic_array_t arr, size_t index, Team elem) {
  __internal_insert(arr, index, &elem);
}

struct file{
string Filename;
string Data;
int Mode;
};
enum file_mode {
READ_MODE,
WRITE_MODE,
CLOSED};
file open_file(string filename, file_mode mode)
{string __strtmp_0; __rock_make_string(&__strtmp_0, "", 0);
struct file tmp_res = {
.Filename = filename,
.Data = __strtmp_0,
.Mode = mode};
file res = allocate_compiler_persistent(sizeof(struct file));
*res = tmp_res;
if (mode == READ_MODE)
{
res->Data = read_file(filename);
}
else
{
if (mode != WRITE_MODE)
{string __strtmp_1; __rock_make_string(&__strtmp_1, "Could not open file \'", 21);
print(__strtmp_1);
print(filename);
string __strtmp_2; __rock_make_string(&__strtmp_2, "\': Invalid mode specified\n", 26);
print(__strtmp_2);
exit(1);
}}
return res;
}

void close_file(file f)
{if (f->Mode == WRITE_MODE)
{
write_string_to_file(f->Data, f->Filename);
}
string __strtmp_3; __rock_make_string(&__strtmp_3, "", 0);
f->Data = __strtmp_3;
string __strtmp_4; __rock_make_string(&__strtmp_4, "", 0);
f->Filename = __strtmp_4;
f->Mode = CLOSED;
}

string get_file_contents(file f)
{if (f->Mode != READ_MODE)
{