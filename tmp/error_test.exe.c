#include "alloc.h"
#include "fundefs.h"
#include "fundefs_internal.h"
#include "typedefs.h"

int main(int argc, char **argv) {
init_compiler_stack();
fill_cmd_args(argc, argv);
{