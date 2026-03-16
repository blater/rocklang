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
  FILE *pre_f;          // Buffer for pre-statements (ZXN statement splitting)
  char *pre_buf;        // Contents of pre_f buffer
  size_t pre_buf_size;  // Size of pre_buf
  char *lib_path;       // Path to library headers (absolute)
} generator_t;

generator_t new_generator(char *filename, char *lib_path);
void kill_generator(generator_t g);

void transpile(generator_t *g, ast_t program);

#endif // GENERATOR_H
