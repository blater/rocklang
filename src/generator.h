/*****************************************************
 * ROCKER GENERATOR HEADER
 * MIT License
 * Copyright (c) 2024 Paul Passeron
 *****************************************************/

#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"
#include "name_table.h"
#include <stdio.h>

typedef enum {
  TARGET_HOST,
  TARGET_ZXN
} target_t;

typedef struct generator_t generator_t;

typedef struct generator_t {
  FILE *f;
  name_table_t table;
  int str_tmp_counter;
  target_t target;
  FILE *pre_f;               // Buffer for pre-statements (ZXN statement splitting)
  char *pre_buf;             // Contents of pre_f buffer
  size_t pre_buf_size;       // Size of pre_buf
  string_view current_module_type; // Set while generating a module method body
  int in_global_scope;       // 1 when emitting top-level statements, 0 inside functions
  ast_array_t deferred_module_inits; // global module vars needing _new() in main()
  char **deferred_global_init_code; // runtime-init code to emit at start of main()
  int deferred_global_init_count;
  int deferred_global_init_capacity;
  ast_t program;             // top-level program node, set at top of transpile()
} generator_t;

generator_t new_generator(char *filename);
void kill_generator(generator_t g);

void transpile(generator_t *g, ast_t program);

#endif // GENERATOR_H
