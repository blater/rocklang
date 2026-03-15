#include "alloc.h"
#include "fundefs.h"
#include "fundefs_internal.h"
#include "typedefs.h"

int main(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
  {
    int a = -(1);
    int b = -(-(2));
    print_int(a);
    print_int(b);
  }
  kill_compiler_stack();
  return 0;
}
