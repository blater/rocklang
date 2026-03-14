#include "alloc.h"
#include "error.h"
#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
// #include "typechecker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// #include "generator.h"

void usage(char *name) {
  printf("Usage:\n");
  printf("\t%s <input file> [output file] [options]\n", name);
  printf("Options:\n");
  printf("\t--target=zxn\t\tCompile for ZX Spectrum Next\n");
  // printf("\t%s [flags] <input file> [output file] [flags]\n", name);
  // printf("Possible flags:\n");
  // printf("\t-t:\t\tPrints the ast\n");
  // printf("\t-l:\t\tPrints the list of lexemes\n");
}

int main(int argc, char *argv[]) {
  init_compiler_stack();
  if (argc < 2) {
    usage(argv[0]);
    exit(1);
  }
  char *input = NULL;
  char *output = NULL;
  // int print_tree = 0;
  int print_lexer = 0;
  int target_zxn = 0;

  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];
    if (*arg == '-') {
      // This is a flag
      if (strlen(arg) == 1) {
        printf("Unexpected empty flag !\n");
        usage(argv[0]);
        exit(1);
      }
      // Check for --target=zxn
      else if (strncmp(arg, "--target=", 9) == 0) {
        if (strcmp(arg + 9, "zxn") == 0) {
          target_zxn = 1;
        } else {
          printf("Unknown target '%s'!\n", arg + 9);
          usage(argv[0]);
          exit(1);
        }
      }
      //  else if (*(arg + 1) == 't' && !print_tree)
      //   print_tree = 1;
      else if (*(arg + 1) == 'l' && !print_lexer)
        print_lexer = 1;
      else {
        printf("Unknown flag '%s'!\n", arg + 1);
        usage(argv[0]);
        exit(1);
      }

    } else if (input == NULL)
      input = arg;
    else if (output == NULL) {
      output = arg;
    } else {
      printf("Unexpected argument '%s' !\n", arg);
      usage(argv[0]);
      exit(1);
    }
  }
  if (input == NULL) {
    printf("Expected input !\n");
    usage(argv[0]);
    exit(1);
  }
  if (output == NULL)
    output = "out";

  lexer_t l = new_lexer(input);
  token_array_t prog = lex_program(&l);
  // if (print_lexer)
  //   print_token_array(prog);
  parser_t p = new_parser(prog);
  p.source = l.data;
  p.source_length = l.length;
  parse_program(&p);

  // // type checking
  // int typecheck = tc_program(p.prog);
  // if (!typecheck) {
  //   printf("The program does not type checks\n");
  // }
  // if (print_tree)
  //   print_ast(p.prog);

  char *cout = allocate_compiler_persistent(strlen(output) + 3);
  sprintf(cout, "%s.c", output);
  generator_t g = new_generator(cout);
  transpile(&g, p.prog);
  kill_generator(g);

  // If there were any compilation errors, exit without running C compiler
  if (get_error_count() > 0) {
    kill_compiler_stack();
    return 1;
  }

  // compile C file
  char command[1024];
  fclose(fopen(output, "w"));


  if (target_zxn) {
    // target: zx next.  For ZX Spectrum Next we use z88dk's zcc
    sprintf(command,
            "zcc +zxn -subtype=nex -startup=0 -clib=sdcc_iy "
            "-pragma-include:lib/zxn/zpragma_zxn.inc -create-app "
            "-I lib/cpu_agnostic -I lib/z80 "
            "-o %s %s "
            "lib/cpu_agnostic/alloc.c "
            "lib/cpu_agnostic/fundefs.c "
            "lib/z80/asm_interop.c",
            output, cout);
  } else {
    // target: host machine with gcc
    sprintf(command,
            "clang-format %s -i && "
            "gcc -Wall -g -I lib/cpu_agnostic -I lib/z80 -o %s %s "
            "lib/cpu_agnostic/alloc.c "
            "lib/cpu_agnostic/fundefs.c "
            "lib/cpu_agnostic/fundefs_internal.c "
            "lib/z80/asm_interop.c",
            cout, output, cout);
  }

  // compile the generated C
  printf("[COMPILATION CMD] %s\n", command);
  system(command);

  // clean up
  kill_compiler_stack();
  return 0;
}
