#include "alloc.h"
#include "fundefs.h"
#include "fundefs_internal.h"
#include "typedefs.h"

int main(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
  {
    string __strtmp_0;
    __rock_make_string(&__strtmp_0, "hello", 5);
    string x = __strtmp_0;
    print(x);
  }
  kill_compiler_stack();
  return 0;
}
