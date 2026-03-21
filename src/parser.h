/*****************************************************
 * ROCKER PARSER HEADER
 * MIT License
 * Copyright (c) 2024 Paul Passeron
 *****************************************************/

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "token.h"

typedef struct parser_t {
  token_array_t tokens;
  ast_t prog;
  int cursor;
  char *source;         // Raw source data for embed block processing
  int source_length;    // Length of raw source data
  int scope_depth;      // 0 = top-level, >0 = inside function/block
} parser_t;

parser_t new_parser(token_array_t l);
token_type_t peek_type(parser_t p);
token_t peek_token(parser_t p);
void parse_program(parser_t* p);
token_t consume_token(parser_t* p);
void set_include_base_dir(char *dir);
#endif  // PARSER_H
