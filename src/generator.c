//-----------------------------------------------------------------------------
//  ROCKER GENERATOR
//  MIT License
//  Copyright (c) 2024 Paul Passeron
//-----------------------------------------------------------------------------

#include "generator.h"
#include "alloc.h"
#include "ast.h"
#include "error.h"
#include "name_table.h"
#include "stringview.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void generate_statement(generator_t *g, ast_t stmt);
void generate_compound(generator_t *g, ast_t comp);
void generate_tdef(generator_t *g, ast_t tdef_ast);
void generate_fundef(generator_t *g, ast_t fun);
int is_builtin_typename(char *name);
void generate_sub_as_expression(generator_t *g, ast_t expr);
void generate_assignement(generator_t *g, ast_t assignment);
void generate_iter_loop(generator_t *g, ast_t loop);
void generate_embed(generator_t *g, ast_t node);
void write_allocator_code(FILE *f);
void generate_expression(generator_t *g, ast_t expr);
const char *allocate_string_tmp(generator_t *g);
string_view get_var_type(string_view name, name_table_t table);
string_view get_array_element_type(ast_t arr, name_table_t table, token_t call_token);
int get_literal_string_length(token_t tok);

// Helper: Flush accumulated pre-statements to destination and reset pre_f
static void flush_pre_f(generator_t *g, FILE *dest);

// Helper: Capture expression into a temporary buffer and return it
// Caller must free() the returned buffer
static char* capture_expression(generator_t *g, ast_t expr) {
  char *buf = NULL;
  size_t size = 0;
  FILE *f = open_memstream(&buf, &size);
  FILE *saved_f = g->f;
  g->f = f;
  generate_expression(g, expr);
  fflush(f);
  fclose(f);
  g->f = saved_f;
  return buf;
}

// Helper: Create a synthetic AST type node for registering builtin signatures
static ast_t make_type_node(const char *type_name) {
  node_t n = {0};
  n.tag = type;
  n.data.type.name.lexeme = sv_from_cstr((char*)type_name);
  return new_ast(n);
}

// Helper: Register a builtin function in the name table with its return type
static void register_builtin(name_table_t *table, const char *name,
                              const char *ret_type) {
  node_t fn = {0};
  fn.tag = fundef;
  fn.data.fundef.name.lexeme = sv_from_cstr((char*)name);
  fn.data.fundef.ret_type = make_type_node(ret_type);
  push_nt(table, sv_from_cstr((char*)name), NT_FUN, new_ast(fn));
}

// Helper: Check if a function is a known operation that's always available
// These functions emit code patterns we recognize regardless of name table
// Unified string concat: capture arguments, emit setup to pre_f, emit tmp var
void emit_concat(generator_t *g, ast_funcall call) {
  ast_t arg = call.args.data[1];
  int is_char = 0;

  if (arg->tag == literal && arg->data.literal.lit.type == TOK_CHR_LIT) {
    is_char = 1;
  } else if (arg->tag == identifier) {
    string_view type = get_var_type(arg->data.identifier.id.lexeme, g->table);
    if (svcmp(type, sv_from_cstr("char")) == 0) is_char = 1;
  } else if (arg->tag == arr_index) {
    string_view elem_type = get_array_element_type(arg->data.arr_index.array, g->table, call.name);
    if (svcmp(elem_type, sv_from_cstr("char")) == 0) is_char = 1;
  } else if (arg->tag == funcall) {
    string_view fname = arg->data.funcall.name.lexeme;
    if ((svcmp(fname, sv_from_cstr("get")) == 0 || svcmp(fname, sv_from_cstr("pop")) == 0) &&
        arg->data.funcall.args.length > 0) {
      string_view elem_type = get_array_element_type(arg->data.funcall.args.data[0], g->table, call.name);
      if (svcmp(elem_type, sv_from_cstr("char")) == 0) is_char = 1;
    }
  }

  const char *func_name = is_char ? "__concat_char" : "__concat_str";
  const char *tmp_var = allocate_string_tmp(g);

  char *arg0_buf = capture_expression(g, call.args.data[0]);
  char *arg1_buf = capture_expression(g, call.args.data[1]);

  fprintf(g->pre_f, "string %s; %s(&%s, %s, %s);\n",
          tmp_var, func_name, tmp_var, arg0_buf, arg1_buf);
  fprintf(g->f, "%s", tmp_var);

  free(arg0_buf);
  free(arg1_buf);
  free((char*)tmp_var);
}

void emit_to_string(generator_t *g, ast_funcall call) {
  // Capture argument, emit setup to pre_f, emit tmp var to main output
  ast_t arg = call.args.data[0];
  const char *fn = "__to_string_int";

  if (arg->tag == identifier) {
    string_view type = get_var_type(arg->data.identifier.id.lexeme, g->table);
    if (svcmp(type, sv_from_cstr("byte")) == 0) fn = "__to_string_byte";
    else if (svcmp(type, sv_from_cstr("word")) == 0) fn = "__to_string_word";
    else if (svcmp(type, sv_from_cstr("dword")) == 0) fn = "__to_string_dword";
  }

  const char *tmp_var = allocate_string_tmp(g);
  char *arg_buf = capture_expression(g, arg);

  fprintf(g->pre_f, "string %s; %s(&%s, %s);\n", tmp_var, fn, tmp_var, arg_buf);
  fprintf(g->f, "%s", tmp_var);

  free(arg_buf);
  free((char*)tmp_var);
}

void emit_substring(generator_t *g, ast_funcall call) {
  // Capture arguments, emit setup to pre_f, emit tmp var to main output
  const char *tmp_var = allocate_string_tmp(g);

  if (call.args.length == 2) {
    char *arg0_buf = capture_expression(g, call.args.data[0]);
    char *arg1_buf = capture_expression(g, call.args.data[1]);

    fprintf(g->pre_f, "string %s; __substring_from(&%s, %s, %s);\n",
            tmp_var, tmp_var, arg0_buf, arg1_buf);
    fprintf(g->f, "%s", tmp_var);

    free(arg0_buf);
    free(arg1_buf);
    free((char*)tmp_var);
  } else if (call.args.length == 3) {
    char *arg0_buf = capture_expression(g, call.args.data[0]);
    char *arg1_buf = capture_expression(g, call.args.data[1]);
    char *arg2_buf = capture_expression(g, call.args.data[2]);

    fprintf(g->pre_f, "string %s; __substring_range(&%s, %s, %s, %s);\n",
            tmp_var, tmp_var, arg0_buf, arg1_buf, arg2_buf);
    fprintf(g->f, "%s", tmp_var);

    free(arg0_buf);
    free(arg1_buf);
    free(arg2_buf);
    free((char*)tmp_var);
  } else {
    token_t tok = call.name;
    error(tok.filename, tok.line, tok.col, "substring() requires 2 or 3 arguments, got %d",
          call.args.length);
  }
}

void emit_string_literal(generator_t *g, const char *tmp_var, token_t tok) {
  // Write string setup to pre_f, emit just variable name to main output
  fprintf(g->pre_f, "string %s; __rock_make_string(&%s, " SV_Fmt ", %d);\n",
          tmp_var, tmp_var, SV_Arg(tok.lexeme),
          get_literal_string_length(tok) - 1);
  fprintf(g->f, "%s", tmp_var);
}

// Helper: Flush accumulated pre-statements to destination and reset pre_f
// (used in Step 3 for ZXN statement splitting)
__attribute__((unused))
static void flush_pre_f(generator_t *g, FILE *dest) {
  if (g->pre_f == NULL)
    return;
  fflush(g->pre_f);
  if (g->pre_buf && g->pre_buf_size > 0)
    fprintf(dest, "%s", g->pre_buf);
  // Reset pre_f for next batch of statements
  fclose(g->pre_f);
  free(g->pre_buf);
  g->pre_buf = NULL;
  g->pre_buf_size = 0;
  g->pre_f = open_memstream(&g->pre_buf, &g->pre_buf_size);
}

generator_t new_generator(char *filename) {
  generator_t res;
  res.f = fopen(filename, "wb");
  if (res.f == NULL)
    perror("Could not open file !");
  res.table = new_name_table();
  res.str_tmp_counter = 0;
  res.target = TARGET_HOST;

  // Register C library builtin functions with their return types
  // Stdlib / I/O
  register_builtin(&res.table, "print",                "void");
  register_builtin(&res.table, "printf",               "void");
  // Array operations - handled specially by name lookup in generator
  register_builtin(&res.table, "length",               "int");
  // String operations from fundefs.h
  register_builtin(&res.table, "get_nth_char",         "char");
  register_builtin(&res.table, "set_nth_char",         "void");
  register_builtin(&res.table, "get_string_length",    "int");
  register_builtin(&res.table, "str_eq",               "int");
  register_builtin(&res.table, "string_to_cstr",       "void");
  register_builtin(&res.table, "cstr_to_string",       "string");
  register_builtin(&res.table, "new_string",           "string");
  // File operations
  register_builtin(&res.table, "read_file",            "string");
  register_builtin(&res.table, "write_string_to_file", "void");
  register_builtin(&res.table, "get_abs_path",         "string");
  // Numeric conversions
  register_builtin(&res.table, "to_int",               "int");
  register_builtin(&res.table, "to_byte",              "byte");
  register_builtin(&res.table, "to_word",              "word");
  register_builtin(&res.table, "to_dword",             "dword");
  register_builtin(&res.table, "set_string_index_base","void");
  // Built-in array/string functions with special code generation
  register_builtin(&res.table, "substring",            "string");
  register_builtin(&res.table, "concat",               "string");
  register_builtin(&res.table, "to_string",            "string");
  register_builtin(&res.table, "toString",             "string");
  // Core compiler functions - always available
  register_builtin(&res.table, "exit",                 "void");
  register_builtin(&res.table, "putchar",              "void");
  register_builtin(&res.table, "allocate_compiler_persistent", "void");
  register_builtin(&res.table, "fill_cmd_args",        "void");
  // Memory operations
  register_builtin(&res.table, "poke",                 "void");
  register_builtin(&res.table, "peek",                 "byte");

  // Always initialize pre_f buffer for statement splitting
  res.pre_buf = NULL;
  res.pre_buf_size = 0;
  res.pre_f = open_memstream(&res.pre_buf, &res.pre_buf_size);
  if (res.pre_f == NULL)
    perror("Could not open memstream for pre_f!");

  return res;
}

void kill_generator(generator_t g) {
  fclose(g.f);
  // Clean up pre_f buffer if initialized (ZXN target only)
  if (g.pre_f != NULL) {
    fflush(g.pre_f);
    fclose(g.pre_f);
  }
}

name_table_t new_name_table(void) {
  name_table_t res;
  res.length = 0;
  res.capacity = INIT_NT_CAP;
  res.scope = 0;
  res.refs = new_ast_array();
  res.kinds = allocate_compiler_persistent(sizeof(nt_kind) * res.capacity);
  res.names = allocate_compiler_persistent(sizeof(char *) * res.capacity);
  res.scopes = allocate_compiler_persistent(sizeof(int) * res.capacity);
  return res;
}

void generate_type(FILE *f, ast_t a) {
  ast_type type = a->data.type;
  if (type.is_array)
    fprintf(f, "__internal_dynamic_array_t");
  else
    fprintf(f, SV_Fmt, SV_Arg(type.name.lexeme));
}

// Helper: Extract type from a variable identifier (used by array operations)
// Returns type name on success, exits on error
string_view get_array_var_type(string_view var_name, name_table_t table,
                                token_t tok) {
  ast_t ref = get_ref(var_name, table);
  if (ref == NULL) {
    error(tok.filename, tok.line, tok.col, "Array is not declared in the current scope");
  }
  if (ref->tag != vardef) {
    error(tok.filename, tok.line, tok.col, "Arrays must be declared as variables (got tag %d)",
          ref->tag);
  }
  ast_type type = ref->data.vardef.type->data.type;
  return type.name.lexeme;
}

// Helper: Get the declared type of a variable identifier
// Returns type name as string_view, or empty string_view if not found
string_view get_var_type(string_view name, name_table_t table) {
  ast_t ref = get_ref(name, table);
  if (!ref) return sv_from_cstr("");

  if (ref->tag == vardef) {
    ast_type type = ref->data.vardef.type->data.type;
    return type.name.lexeme;
  }

  if (ref->tag == fundef) {
    // Look up parameter type in function definition
    ast_fundef fundef = ref->data.fundef;
    for (int i = 0; i < fundef.args.length; i++) {
      if (svcmp(fundef.args.data[i].lexeme, name) == 0) {
        ast_type type = fundef.types.data[i]->data.type;
        return type.name.lexeme;
      }
    }
  }

  return sv_from_cstr("");
}

// Helper: Determine if an expression returns a string type
// Returns 1 if expression type is "string", 0 otherwise
int expr_returns_string(ast_t expr, name_table_t table) {
  if (!expr) return 0;

  // String literals always return string type
  if (expr->tag == literal) {
    return expr->data.literal.lit.type == TOK_STR_LIT;
  }

  if (expr->tag == funcall) {
    // Function call - check return type
    ast_funcall call = expr->data.funcall;
    ast_t func_ref = get_ref(call.name.lexeme, table);

    if (func_ref && func_ref->tag == fundef) {
      ast_fundef fundef = func_ref->data.fundef;
      if (fundef.ret_type) {
        ast_type ret_type = fundef.ret_type->data.type;
        return svcmp(ret_type.name.lexeme, sv_from_cstr("string")) == 0;
      }
    }
    return 0;
  }

  if (expr->tag == identifier) {
    // Variable - check its type
    string_view type = get_var_type(expr->data.identifier.id.lexeme, table);
    return svcmp(type, sv_from_cstr("string")) == 0;
  }

  // Other expression types don't return strings for our purposes
  return 0;
}

// Helper: Convert a Rock string expression to C string (const char*)
// Rock strings are null-terminated, so we extract the .data field
void generate_string_to_cstr(generator_t *g, ast_t expr) {
  generate_expression(g, expr);
  fprintf(g->f, ".data");
}

// Helper: Get element type for struct field access (::)
// Handles known type/field combinations
// Returns element type on success, or empty string if unknown
string_view try_get_field_array_type(string_view base_type,
                                      string_view field_name,
                                      name_table_t table) {
  // Special case: tdef_ast::constructors is ast[]
  if (svcmp(base_type, sv_from_cstr("tdef_ast")) == 0) {
    if (svcmp(field_name, sv_from_cstr("constructors")) == 0) {
      return sv_from_cstr("ast");
    }
  }

  // Special case: type_spec::constructors is constructor_spec[]
  if (svcmp(base_type, sv_from_cstr("type_spec")) == 0) {
    if (svcmp(field_name, sv_from_cstr("constructors")) == 0) {
      return sv_from_cstr("constructor_spec");
    }
  }

  // Special case: ast_array_t and similar types
  if (svcmp(base_type, sv_from_cstr("ast_array_t")) == 0 ||
      svcmp(base_type, sv_from_cstr("ast")) == 0 ||
      svcmp(base_type, sv_from_cstr("ast []")) == 0) {
    return sv_from_cstr("ast");
  }

  // Special case: token_array_t fields
  if (svcmp(base_type, sv_from_cstr("token_array_t")) == 0 ||
      svcmp(base_type, sv_from_cstr("token []")) == 0) {
    return sv_from_cstr("token");
  }

  // Try to look up user-defined struct types
  ast_t tdef_ref = get_ref(base_type, table);
  if (tdef_ref && tdef_ref->tag == tdef) {
    ast_tdef tdef = tdef_ref->data.tdef;
    // Search constructors (fields) for matching field name
    for (int i = 0; i < tdef.constructors.length; i++) {
      ast_t cons_ast = tdef.constructors.data[i];
      if (cons_ast->tag == cons) {
        ast_cons cons = cons_ast->data.cons;
        if (svcmp(cons.name.lexeme, field_name) == 0) {
          // Found the field - check if its type is an array
          if (cons.type && cons.type->tag == type) {
            ast_type field_type = cons.type->data.type;
            if (field_type.is_array) {
              // Return the element type
              return field_type.name.lexeme;
            }
          }
          // Field exists but is not an array
          return sv_from_cstr("");
        }
      }
    }
  }

  // If field ends with 's' or contains 'array', might be array
  // For now, default to empty string for unknown cases
  return sv_from_cstr("");
}

// Helper: Determine element type of an array expression
// Handles identifiers, field access (::), and known function calls (like get_args)
// call_token: fallback token for error reporting (e.g., the function call token)
// Returns type name on success, exits on error
string_view get_array_element_type(ast_t arr, name_table_t table,
                                    token_t call_token) {
  if (arr->tag == identifier) {
    token_t tok = arr->data.identifier.id;
    string_view name = tok.lexeme;
    return get_array_var_type(name, table, tok);
  } else if (arr->tag == sub) {
    // Handle field access: base::field
    ast_sub sub = arr->data.sub;

    // The path contains the base identifier(s)
    // The expr contains the field being accessed
    string_view base_type = sv_from_cstr("");
    string_view field_name = sv_from_cstr("");

    // Get the base type from the first identifier in path
    if (sub.path.length > 0) {
      string_view base_name = sub.path.data[0].lexeme;
      ast_t base_ref = get_ref(base_name, table);

      // Extract base type from either vardef or fundef parameter
      if (base_ref) {
        if (base_ref->tag == vardef) {
          base_type = base_ref->data.vardef.type->data.type.name.lexeme;
        } else if (base_ref->tag == fundef) {
          // Function parameter: find which arg matches base_name
          ast_fundef fundef = base_ref->data.fundef;
          for (int i = 0; i < fundef.args.length; i++) {
            if (svcmp(fundef.args.data[i].lexeme, base_name) == 0) {
              base_type = fundef.types.data[i]->data.type.name.lexeme;
              break;
            }
          }
        }
      }
    }

    // Get the field name from expr (should be an identifier)
    if (sub.expr && sub.expr->tag == identifier) {
      field_name = sub.expr->data.identifier.id.lexeme;
    }

    // Look up the field type if we have both base type and field name
    if (base_type.length > 0 && field_name.length > 0) {
      string_view field_type = try_get_field_array_type(base_type, field_name, table);

      // If we found a known field type, return it
      if (field_type.length > 0) {
        return field_type;
      }
    }

    // If we couldn't determine the type, error out
    error(call_token.filename, call_token.line, call_token.col,
          "Cannot infer array type from field access expression; array element type must be determinable from an identifier or known function call");
    return sv_from_cstr("");
  } else if (arr->tag == funcall) {
    // For function calls, try to infer type from function name
    token_t tok = arr->data.funcall.name;
    string_view func_name = tok.lexeme;
    if (svcmp(func_name, sv_from_cstr("get_args")) == 0) {
      return sv_from_cstr("string");
    } else {
      error(tok.filename, tok.line, tok.col, "Cannot infer array type from function call: " SV_Fmt,
            SV_Arg(func_name));
      return sv_from_cstr("");
    }
  } else {
    // Try to extract location info from the expression itself, fallback to call_token
    char *filename = call_token.filename;
    int line = call_token.line;
    int col = call_token.col;

    // Try to get more precise info from various expression types
    if (arr->tag == literal) {
      filename = arr->data.literal.lit.filename;
      line = arr->data.literal.lit.line;
      col = arr->data.literal.lit.col;
    } else if (arr->tag == op) {
      // For operators, use left operand's location
      if (arr->data.op.left && arr->data.op.left->tag == literal) {
        filename = arr->data.op.left->data.literal.lit.filename;
        line = arr->data.op.left->data.literal.lit.line;
        col = arr->data.op.left->data.literal.lit.col;
      }
    }

    error(filename, line, col,
          "Array argument must be an identifier, field access (::), or get_args() call; got unsupported expression type");
    return sv_from_cstr("");
  }
}

void generate_expression(generator_t *g, ast_t expr);

void generate_subscript(generator_t *g, ast_t expr) {
  FILE *f = g->f;
  ast_arr_index sub = expr->data.arr_index;
  token_t call_token = sub.array->data.identifier.id;
  string_view elem_type = get_array_element_type(sub.array, g->table, call_token);

  // For string arrays, use pre_f to emit setup statements
  if (svcmp(elem_type, sv_from_cstr("string")) == 0) {
    const char *tmp = allocate_string_tmp(g);
    char *arr_buf = capture_expression(g, sub.array);
    char *idx_buf = capture_expression(g, sub.index);
    fprintf(g->pre_f, "string %s; string_get_elem(&%s, %s, (size_t)(%s));\n",
            tmp, tmp, arr_buf, idx_buf);
    fprintf(f, "%s", tmp);
    free(arr_buf);
    free(idx_buf);
    // Handle field access if present
    if (sub.has_field) {
      for (int i = 0; i < sub.field_path.length; i++)
        fprintf(f, "->" SV_Fmt, SV_Arg(sub.field_path.data[i].lexeme));
      fprintf(f, "->");
      generate_expression(g, sub.field_expr);
    }
    free((char*)tmp);
  } else {
    // Non-string arrays: emit directly as before
    fprintf(f, SV_Fmt "_get_elem(", SV_Arg(elem_type));
    generate_expression(g, sub.array);
    fprintf(f, ", (size_t)(");
    generate_expression(g, sub.index);
    fprintf(f, "))");

    // Emit: ->field0->field1->...->field_expr
    if (sub.has_field) {
      for (int i = 0; i < sub.field_path.length; i++)
        fprintf(f, "->" SV_Fmt, SV_Arg(sub.field_path.data[i].lexeme));
      fprintf(f, "->");
      generate_expression(g, sub.field_expr);
    }
  }
}

// Metadata for array operations
typedef struct {
  int expected_args;
  const char *suffix;
  const char *name;
} array_op_t;

// Unified code generator for all array operations (append, get, set, pop)
void generate_array_op(generator_t *g, ast_funcall call, array_op_t op) {
  FILE *f = g->f;

  // Validate argument count
  if (call.args.length != op.expected_args) {
    error(call.name.filename, call.name.line, call.name.col,
          "%s() requires %d argument(s), got %d", op.name,
          op.expected_args, call.args.length);
    return;
  }

  // Determine element type (pass call.name as fallback for error reporting)
  string_view type_name =
      get_array_element_type(call.args.data[0], g->table, call.name);

  // For string arrays with get_elem or pop_array, use pre_f for setup statements
  if (svcmp(type_name, sv_from_cstr("string")) == 0 &&
      (strcmp(op.suffix, "_get_elem") == 0 || strcmp(op.suffix, "_pop_array") == 0)) {
    const char *tmp = allocate_string_tmp(g);
    if (strcmp(op.suffix, "_get_elem") == 0) {
      // string_get_elem: 2 args (array, index)
      char *arr_buf = capture_expression(g, call.args.data[0]);
      char *idx_buf = capture_expression(g, call.args.data[1]);
      fprintf(g->pre_f, "string %s; string_get_elem(&%s, %s, (size_t)(%s));\n",
              tmp, tmp, arr_buf, idx_buf);
      fprintf(f, "%s", tmp);
      free(arr_buf);
      free(idx_buf);
    } else {
      // string_pop_array: 1 arg (array)
      char *arr_buf = capture_expression(g, call.args.data[0]);
      fprintf(g->pre_f, "string %s; string_pop_array(&%s, %s);\n",
              tmp, tmp, arr_buf);
      fprintf(f, "%s", tmp);
      free(arr_buf);
    }
    free((char*)tmp);
  } else {
    // Non-string or void-returning operations: emit directly
    fprintf(f, SV_Fmt "%s(", SV_Arg(type_name), op.suffix);
    for (int i = 0; i < call.args.length; i++) {
      if (i > 0)
        fprintf(f, ", ");
      generate_expression(g, call.args.data[i]);
    }
    fprintf(f, ")");
  }
}

// Convenience wrappers for each array operation
void generate_append(generator_t *g, ast_funcall call) {
  array_op_t op = {2, "_push_array", "append"};
  generate_array_op(g, call, op);
}

void generate_get(generator_t *g, ast_funcall call) {
  array_op_t op = {2, "_get_elem", "get"};
  generate_array_op(g, call, op);
}

void generate_set(generator_t *g, ast_funcall call) {
  array_op_t op = {3, "_set_elem", "set"};
  generate_array_op(g, call, op);
}

void generate_pop(generator_t *g, ast_funcall call) {
  array_op_t op = {1, "_pop_array", "pop"};
  generate_array_op(g, call, op);
}

void generate_insert(generator_t *g, ast_funcall call) {
  array_op_t op = {3, "_insert", "insert"};
  generate_array_op(g, call, op);
}

// Helper: Allocate a temporary string variable and return its name
const char *allocate_string_tmp(generator_t *g) {
  char tmpname_buf[64];
  snprintf(tmpname_buf, sizeof(tmpname_buf), "__strtmp_%d", g->str_tmp_counter);
  g->str_tmp_counter++;
  return strdup(tmpname_buf);
}

void generate_substring(generator_t *g, ast_funcall call) {
  if (call.args.length != 2 && call.args.length != 3) {
    token_t tok = call.name;
    error(tok.filename, tok.line, tok.col,
          "substring() requires 2 or 3 arguments, got %d", call.args.length);
    return;
  }
  emit_substring(g, call);
}

void generate_concat(generator_t *g, ast_funcall call) {
  if (call.args.length != 2) {
    error(call.name.filename, call.name.line, call.name.col, "concat() requires 2 arguments, got %d",
          call.args.length);
  }
  emit_concat(g, call);
}

void generate_to_string(generator_t *g, ast_funcall call) {
  if (call.args.length != 1) {
    error(call.name.filename, call.name.line, call.name.col, "to_string() requires 1 argument, got %d",
          call.args.length);
  }
  emit_to_string(g, call);
}

void generate_funcall(generator_t *g, ast_t fun) {
  FILE *f = g->f;
  ast_funcall funcall = fun->data.funcall;
  if (svcmp(funcall.name.lexeme, sv_from_cstr("append")) == 0) {
    generate_append(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("get")) == 0) {
    generate_get(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("set")) == 0) {
    generate_set(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("pop")) == 0) {
    generate_pop(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("insert")) == 0) {
    generate_insert(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("substring")) == 0) {
    generate_substring(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("concat")) == 0) {
    generate_concat(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("to_string")) == 0 ||
             svcmp(funcall.name.lexeme, sv_from_cstr("toString")) == 0) {
    generate_to_string(g, funcall);
  } else if (svcmp(funcall.name.lexeme, sv_from_cstr("printf")) == 0) {
    // Rock printf takes one string argument
    FILE *f = g->f;
    ast_t arg = funcall.args.data[0];

    // String expression: wrap with "%s" format and extract .data
    if (expr_returns_string(arg, g->table)) {
      fprintf(f, "printf(\"%%s\", ");
      generate_string_to_cstr(g, arg);
      fprintf(f, ")");
    }
    // Non-string: just emit it
    else {
      fprintf(f, "printf(");
      generate_expression(g, arg);
      fprintf(f, ")");
    }
  } else {
    // Check if function is defined (either in name table or as a special builtin)
    string_view fname = funcall.name.lexeme;
    ast_t func_ref = get_ref(fname, g->table);

    // Function must be defined in name table
    if (func_ref == NULL) {
      error(funcall.name.filename, funcall.name.line, funcall.name.col,
            "undefined function " SV_Fmt, SV_Arg(fname));
    }

    // Emit plain function call
    fprintf(f, SV_Fmt "(", SV_Arg(funcall.name.lexeme));
    for (int i = 0; i < funcall.args.length; i++) {
      if (i > 0)
        fprintf(f, ", ");
      generate_expression(g, funcall.args.data[i]);
    }
    fprintf(f, ")");
  }
}

int calcEscapedLength(const char *str) {
  int length = 0;
  int i = 0;
  while (str[i]) {
    if (str[i] == '\\') { // Check if it is an escape character
      i++; // Move to the next character to interpret the escape sequence
      if (str[i] == 'n' || str[i] == 't' || str[i] == '\\' || str[i] == '"' ||
          str[i] == '\'' || str[i] == 'r') {
        length++; // These are single character escape sequences
      } else {
        length += 2; // For unrecognized escape sequences, count both characters
      }
    } else {
      length++;
    }
    i++;
  }
  return length - 1;
}

int get_literal_string_length(token_t tok) {
  return calcEscapedLength(string_of_sv(tok.lexeme));
}

void generate_op(generator_t *g, ast_t expr) {
  ast_op op = expr->data.op;
  FILE *f = g->f;
  generate_expression(g, op.left);
  if (op.op == TOK_EQUAL)
    fprintf(f, " == ");
  else
    fprintf(f, " " SV_Fmt " ", SV_Arg(lexeme_of_type(op.op)));
  generate_expression(g, op.right);
}

void generate_unary_op(generator_t *g, ast_t expr) {
  FILE *f = g->f;
  ast_unary_op u = expr->data.unary_op;
  fprintf(f, SV_Fmt "(", SV_Arg(lexeme_of_type(u.op)));
  generate_expression(g, u.operand);
  fprintf(f, ")");
}

void generate_if_statement(generator_t *g, ast_t stmt) {
  FILE *f = g->f;
  ast_ifstmt ifstmt = stmt->data.ifstmt;
  fprintf(f, "if (");
  generate_expression(g, ifstmt.expression);
  fprintf(f, ")\n");

  // Wrap non-compound bodies in braces to allow setup statements
  int wrap_body = (ifstmt.body->tag != compound);
  if (wrap_body) fprintf(f, "{\n");
  generate_statement(g, ifstmt.body);
  if (wrap_body) fprintf(f, "}\n");

  if (ifstmt.elsestmt != NULL) {
    fprintf(f, "else\n");
    int wrap_else = (ifstmt.elsestmt->tag != compound);
    if (wrap_else) fprintf(f, "{\n");
    generate_statement(g, ifstmt.elsestmt);
    if (wrap_else) fprintf(f, "}\n");
  }
}

// Generate all type-specific array helper functions (make_array, push_array, pop_array, etc.)
// This is a template that generates 6 helper functions for a given element type
void generate_array_funcs(generator_t *g, char *type_name) {
  FILE *f = g->f;

  // Template group: make_array, push_array, pop_array, get_elem, set_elem, insert
  // Each generates a wrapper around __internal_* functions with proper type handling

  fprintf(f, "__internal_dynamic_array_t %s_make_array(void) {\n", type_name);
  fprintf(f, "  return __internal_make_array(sizeof(%s), 0);\n", type_name);
  fprintf(f, "}\n\n");

  fprintf(f, "void %s_push_array(__internal_dynamic_array_t arr, %s elem) {\n",
          type_name, type_name);
  fprintf(f, "  __internal_push_array(arr, &elem);\n");
  fprintf(f, "}\n\n");

  fprintf(f, "%s %s_pop_array(__internal_dynamic_array_t arr) {\n", type_name,
          type_name);
  fprintf(f, "  %s *res = __internal_pop_array(arr);\n", type_name);
  fprintf(f, "  return *res;\n");
  fprintf(f, "}\n\n");

  fprintf(f, "%s %s_get_elem(__internal_dynamic_array_t arr, size_t index) {\n",
          type_name, type_name);
  fprintf(f, "  %s *res = __internal_get_elem(arr, index);\n", type_name);
  fprintf(f,
          "  if (res == NULL){ printf(\"NULL ELEMENT IN %s_get_elem\"); "
          "exit(1);}\n",
          type_name);
  fprintf(f, "  return *res;\n");
  fprintf(f, "}\n\n");

  fprintf(f,
          "void %s_set_elem(__internal_dynamic_array_t arr, size_t index, "
          "%s elem) {\n",
          type_name, type_name);
  fprintf(f, "  __internal_set_elem(arr, index, &elem);\n");
  fprintf(f, "}\n\n");

  fprintf(f,
          "void %s_insert(__internal_dynamic_array_t arr, size_t index, "
          "%s elem) {\n",
          type_name, type_name);
  fprintf(f, "  __internal_insert(arr, index, &elem);\n");
  fprintf(f, "}\n\n");
}

void generate_sub(generator_t *g, ast_t sub_ast, int is_rec) {
  FILE *f = g->f;
  ast_sub sub = sub_ast->data.sub;
  assert(sub.path.length == 1 && "TODO; implement nested subs");
  if (is_rec) {
    generate_sub_as_expression(g, sub_ast);
  } else {
    // We check if the expr is a wildcard
    int is_wild = 0;
    if (sub.expr->tag == literal)
      if (sub.expr->data.literal.lit.type == TOK_WILDCARD)
        is_wild = 1;
    fprintf(f, "{.tag = " SV_Fmt, SV_Arg(sub.path.data[0].lexeme));
    if (!is_wild) {
      fprintf(f, ", .data = ");
      generate_expression(g, sub.expr);
    }
    fprintf(f, "}");
  }
}

void generate_loop(generator_t *g, ast_t loop_ast) {
  FILE *f = g->f;
  ast_loop loop = loop_ast->data.loop;
  new_nt_scope(&g->table);
  push_nt(&g->table, loop.variable.lexeme, NT_VAR, loop_ast);
  fprintf(f, "for(int " SV_Fmt " =", SV_Arg(loop.variable.lexeme));
  generate_expression(g, loop.start);
  fprintf(f, "; " SV_Fmt " <= (int)", SV_Arg(loop.variable.lexeme));
  generate_expression(g, loop.end);
  fprintf(f, "; " SV_Fmt "++)\n", SV_Arg(loop.variable.lexeme));

  // Wrap non-compound bodies in braces to allow setup statements
  int wrap_body = (loop.statement->tag != compound);
  if (wrap_body) fprintf(f, "{\n");
  generate_statement(g, loop.statement);
  if (wrap_body) fprintf(f, "}\n");

  end_nt_scope(&g->table);
}

void generate_sub_as_expression(generator_t *g, ast_t expr) {
  FILE *f = g->f;
  ast_sub sub = expr->data.sub;
  // assert(sub.path.length == 1 && "SUB AS EXPRESSION LENGTH LIMIT\n");
  for (int i = 0; i < sub.path.length; i++) {
    fprintf(f, SV_Fmt "->", SV_Arg(sub.path.data[i].lexeme));
  }
  generate_expression(g, sub.expr);
}

void generate_while_loop(generator_t *g, ast_t loop) {
  FILE *f = g->f;
  ast_while_loop while_loop = loop->data.while_loop;
  fprintf(f, "while (");
  generate_expression(g, while_loop.condition);
  fprintf(f, ")\n");

  // Wrap non-compound bodies in braces to allow setup statements
  int wrap_body = (while_loop.statement->tag != compound);
  if (wrap_body) fprintf(f, "{\n");
  generate_statement(g, while_loop.statement);
  if (wrap_body) fprintf(f, "}\n");
}

void generate_iter_loop(generator_t *g, ast_t loop) {
  FILE *f = g->f;
  ast_iter_loop iter_loop = loop->data.iter_loop;
  new_nt_scope(&g->table);

  fprintf(f, "for(int __iter_i = 0; __iter_i < ");
  generate_expression(g, iter_loop.iterable);
  fprintf(f, "->length; __iter_i++)\n");

  fprintf(f, "{\n");

  // Infer the element type from the iterable if it's an identifier
  if (iter_loop.iterable->tag == identifier) {
    string_view iter_name = iter_loop.iterable->data.identifier.id.lexeme;
    ast_t ref = get_ref(iter_name, g->table);
    if (ref != NULL && ref->tag == vardef) {
      // Get the base type (without array brackets)
      ast_vardef vardef = ref->data.vardef;
      ast_type type = vardef.type->data.type;
      // Generate assignment with cast to proper pointer type before indexing
      fprintf(f, SV_Fmt " " SV_Fmt " = ((", SV_Arg(type.name.lexeme), SV_Arg(iter_loop.variable.lexeme));
      fprintf(f, SV_Fmt " *)", SV_Arg(type.name.lexeme));
      generate_expression(g, iter_loop.iterable);
      fprintf(f, "->data)[__iter_i];\n");
    } else {
      // Fallback: can't determine type, assign without type
      fprintf(f, "void *" SV_Fmt " = ", SV_Arg(iter_loop.variable.lexeme));
      generate_expression(g, iter_loop.iterable);
      fprintf(f, "->data[__iter_i];\n");
    }
  } else {
    // If iterable is not an identifier, we can't infer the type
    fprintf(f, "void *" SV_Fmt " = ", SV_Arg(iter_loop.variable.lexeme));
    generate_expression(g, iter_loop.iterable);
    fprintf(f, "->data[__iter_i];\n");
  }

  push_nt(&g->table, iter_loop.variable.lexeme, NT_VAR, loop);
  generate_statement(g, iter_loop.statement);

  fprintf(f, "}\n");
  end_nt_scope(&g->table);
}

void generate_enum_tdef(generator_t *g, ast_t expr) {
  FILE *f = g->f;
  ast_enum_tdef enum_tdef = expr->data.enum_tdef;
  fprintf(f, "enum " SV_Fmt " {\n", SV_Arg(enum_tdef.name.lexeme));
  for (int i = 0; i < enum_tdef.items.length; i++) {
    if (i > 0)
      fprintf(f, ",\n");
    fprintf(f, SV_Fmt, SV_Arg(enum_tdef.items.data[i].lexeme));
  }
  fprintf(f, "};\n");
}

void generate_expression(generator_t *g, ast_t expr) {
  FILE *f = g->f;
  if (expr->tag == literal) {
    token_t tok = expr->data.literal.lit;
    if (tok.type != TOK_STR_LIT)
      fprintf(f, SV_Fmt, SV_Arg(tok.lexeme));
    else {
      const char *tmp_var = allocate_string_tmp(g);
      emit_string_literal(g, tmp_var, tok);
      free((char*)tmp_var);
    }
  } else if (expr->tag == identifier) {
    string_view lexeme = expr->data.identifier.id.lexeme;
    fprintf(f, SV_Fmt, SV_Arg(lexeme));
  } else if (expr->tag == funcall)
    generate_funcall(g, expr);
  else if (expr->tag == op)
    generate_op(g, expr);
  else if (expr->tag == unary_op)
    generate_unary_op(g, expr);
  else if (expr->tag == ifstmt)
    generate_if_statement(g, expr);
  else if (expr->tag == compound)
    generate_compound(g, expr);
  else if (expr->tag == loop)
    generate_loop(g, expr);
  else if (expr->tag == sub)
    generate_sub_as_expression(g, expr);
  else if (expr->tag == assign)
    generate_assignement(g, expr);
  else if (expr->tag == while_loop)
    generate_while_loop(g, expr);
  else if (expr->tag == iter_loop)
    generate_iter_loop(g, expr);
  else if (expr->tag == arr_index)
    generate_subscript(g, expr);

  else {
    printf("TAG is %d\n", expr->tag);
    assert(0 && "TODO");
  }
}

void generate_assignement(generator_t *g, ast_t assignment) {
  FILE *f = g->f;
  ast_assign assign = assignment->data.assign;
  if (assign.target->tag == arr_index) {
    // arr[i] := value  =>  TYPE_set_elem(arr, i, value)
    ast_arr_index sub = assign.target->data.arr_index;
    token_t tok = sub.array->data.identifier.id;
    string_view elem_type = get_array_element_type(sub.array, g->table, tok);
    fprintf(f, SV_Fmt "_set_elem(", SV_Arg(elem_type));
    generate_expression(g, sub.array);
    fprintf(f, ", (size_t)(");
    generate_expression(g, sub.index);
    fprintf(f, "), ");
    generate_expression(g, assign.expr);
    fprintf(f, ")");
  } else {
    generate_expression(g, assign.target);
    fprintf(f, " = ");
    generate_expression(g, assign.expr);
  }
}

void generate_vardef(generator_t *g, ast_t var) {
  FILE *f = g->f;
  ast_vardef vardef = var->data.vardef;
  push_nt(&g->table, vardef.name.lexeme, NT_VAR, var);
  string_view type_name = vardef.type->data.type.name.lexeme;
  if (vardef.type->data.type.is_array) {
    generate_type(f, vardef.type);
    fprintf(f, " " SV_Fmt " = \n", SV_Arg(vardef.name.lexeme));
    if (vardef.expr->tag != literal) {
      generate_expression(g, vardef.expr);
      fprintf(f, ";\n");
    } else if (vardef.expr->data.literal.lit.type != TOK_ARR_DECL) {
      error(vardef.name.filename, vardef.name.line, vardef.name.col,
            "Cannot declare arrays this way yet");
      return;
    } else {
      // Pass array capacity (0 for dynamic, or fixed size)
      int capacity = vardef.type->data.type.array_capacity;
      fprintf(f, "__internal_make_array(sizeof(" SV_Fmt "), %d);\n",
              SV_Arg(type_name), capacity);
    }
  } else if (is_builtin_typename(string_of_sv(type_name))) {
    // Capture the expression reference in a temp buffer while accumulating
    // setup statements to pre_f
    char *expr_text;
    if (vardef.expr->tag == sub)
      expr_text = capture_expression(g, vardef.expr);
    else
      expr_text = capture_expression(g, vardef.expr);

    // Now emit any accumulated setup statements from pre_f
    flush_pre_f(g, f);

    // Finally emit the declaration with the captured expression reference
    generate_type(f, vardef.type);
    fprintf(f, " " SV_Fmt " = %s;\n", SV_Arg(vardef.name.lexeme), expr_text);

    free(expr_text);
  } else if (!is_builtin_typename(string_of_sv(type_name)) &&
             vardef.expr->tag == record_expr) {
    ast_record_expr rec = vardef.expr->data.record_expr;

    // Look up struct type to get field types
    ast_t struct_ref = get_ref(type_name, g->table);

    if (vardef.is_rec) {
      // First, capture all field expressions (which fills pre_f with setup)
      char **field_bufs = malloc(rec.names.length * sizeof(char*));

      for (int i = 0; i < rec.names.length; i++) {
        ast_t expr = rec.exprs.data[i];

        // Check if this field is an array and expression is array literal
        int is_array_field = 0;
        string_view field_element_type = sv_from_cstr("");
        if (struct_ref && struct_ref->tag == tdef) {
          ast_tdef tdef = struct_ref->data.tdef;
          for (int j = 0; j < tdef.constructors.length; j++) {
            ast_t cons_ast = tdef.constructors.data[j];
            if (cons_ast->tag == cons) {
              ast_cons cons = cons_ast->data.cons;
              if (svcmp(cons.name.lexeme, rec.names.data[i].lexeme) == 0) {
                if (cons.type && cons.type->tag == type) {
                  ast_type field_type = cons.type->data.type;
                  if (field_type.is_array) {
                    is_array_field = 1;
                    field_element_type = field_type.name.lexeme;
                  }
                }
                break;
              }
            }
          }
        }

        // Capture the field value
        if (is_array_field && expr->tag == literal &&
            expr->data.literal.lit.type == TOK_ARR_DECL) {
          // Array field with array literal: generate __internal_make_array (dynamic)
          field_bufs[i] = malloc(256);
          snprintf(field_bufs[i], 256, "__internal_make_array(sizeof(" SV_Fmt "), 0)",
                   SV_Arg(field_element_type));
        } else if (expr->tag == sub) {
          field_bufs[i] = capture_expression(g, expr);
        } else {
          field_bufs[i] = capture_expression(g, expr);
        }
      }

      // Flush all setup statements before emitting struct initializer
      flush_pre_f(g, f);

      // Now emit the struct initializer with captured field values
      fprintf(f, "struct ");
      generate_type(f, vardef.type);
      fprintf(f, " tmp_" SV_Fmt " = {\n", SV_Arg(vardef.name.lexeme));
      for (int i = 0; i < rec.names.length; i++) {
        if (i > 0)
          fprintf(f, ",\n");
        fprintf(f, "." SV_Fmt " = %s", SV_Arg(rec.names.data[i].lexeme), field_bufs[i]);
      }
      fprintf(f, "};\n");

      // Free captured field buffers
      for (int i = 0; i < rec.names.length; i++) {
        free(field_bufs[i]);
      }
      free(field_bufs);

    } else {
      if (vardef.expr->tag == sub) {
        generate_sub_as_expression(g, vardef.expr);
      } else {
        generate_expression(g, vardef.expr);
      }
      fprintf(f, ";\n");
    }
    generate_type(f, vardef.type);
    fprintf(f, " " SV_Fmt " = allocate_compiler_persistent(sizeof(struct ",
            SV_Arg(vardef.name.lexeme));
    generate_type(f, vardef.type);
    fprintf(f, "));\n");
    fprintf(f, "*" SV_Fmt " = tmp_" SV_Fmt ";\n", SV_Arg(vardef.name.lexeme),
            SV_Arg(vardef.name.lexeme));
  } else {
    generate_type(f, vardef.type);
    fprintf(f, " " SV_Fmt " = ", SV_Arg(vardef.name.lexeme));
    // Temporary, we'll need to store the constructors in the name table, with
    // a flag saying if it's void or not
    if (vardef.expr->tag == sub)
      generate_sub_as_expression(g, vardef.expr);
    else
      generate_expression(g, vardef.expr);
    fprintf(f, ";\n");
  }
}

void generate_match(generator_t *g, ast_t match) {
  (void)g;
  (void)match;
}

void generate_return(generator_t *g, ast_t ret_ast) {
  FILE *f = g->f;
  // Capture return expression to collect setup statements
  if (ret_ast->data.ret.expr != NULL) {
    char *expr_text = capture_expression(g, ret_ast->data.ret.expr);
    // Flush setup statements before emitting return
    flush_pre_f(g, f);
    fprintf(f, "return %s;\n", expr_text);
    free(expr_text);
  } else {
    // No expression, just return
    fprintf(f, "return;\n");
  }
}

void generate_embed(generator_t *g, ast_t node) {
  FILE *f = g->f;
  ast_embed e = node->data.embed;

  if (strcmp(e.lang, "c") == 0) {
    if (e.is_function) {
      // Emit verbatim at file scope
      fprintf(f, "%s\n", e.body);
    } else {
      // Emit verbatim inside current function scope
      fprintf(f, "{\n%s\n}\n", e.body);
    }
  } else if (strcmp(e.lang, "asm") == 0) {
    // Wrap in GCC inline asm
    fprintf(f, "__asm__ volatile (\"%s\");\n", e.body);
  } else {
    // Unknown language: emit warning
    fprintf(f, "/* WARNING: Unknown embed language '%s' */\n", e.lang);
  }
}

void generate_statement(generator_t *g, ast_t stmt) {
  FILE *f = g->f;
  if (stmt->tag == vardef) {
    generate_vardef(g, stmt);
  } else if (stmt->tag == embed) {
    // Skip top-level embed functions (they're already handled in generate_forward_defs)
    // Only emit inline embed blocks
    ast_embed e = stmt->data.embed;
    if (!e.is_function) {
      generate_embed(g, stmt);
    }
  } else if (stmt->tag == match) {
    assert(0 && "TODO");
    generate_match(g, stmt);
  } else if (stmt->tag == ret) {
    generate_return(g, stmt);
  } else if (stmt->tag == compound) {
    generate_compound(g, stmt);
  } else if (stmt->tag == ifstmt) {
    generate_if_statement(g, stmt);
  } else if (stmt->tag == tdef) {
    generate_tdef(g, stmt);
  } else if (stmt->tag == fundef) {
    generate_fundef(g, stmt);

  } else if (stmt->tag == enum_tdef)
    generate_enum_tdef(g, stmt);
  else if (stmt->tag == iter_loop)
    generate_iter_loop(g, stmt);
  else {
    // For expression statements, capture the expression (which fills pre_f with setup)
    // then flush the setup before emitting the expression
    char *expr_text = capture_expression(g, stmt);
    // Flush any accumulated setup statements from pre_f
    flush_pre_f(g, f);
    // Emit the captured expression and semicolon
    fprintf(f, "%s;\n", expr_text);
    free(expr_text);
  }
}
void generate_compound(generator_t *g, ast_t comp) {
  FILE *f = g->f;
  fprintf(f, "{");
  ast_compound compound = comp->data.compound;
  new_nt_scope(&g->table);
  for (int i = 0; i < compound.stmts.length; i++)
    generate_statement(g, compound.stmts.data[i]);
  end_nt_scope(&g->table);
  fprintf(f, "}");
}

void generate_fundef(generator_t *g, ast_t fun) {
  // add to name table
  FILE *f = g->f;
  ast_fundef fundef = fun->data.fundef;
  new_nt_scope(&g->table);
  push_nt(&g->table, fundef.name.lexeme, NT_FUN, fun);
  if (svcmp(fundef.name.lexeme, sv_from_cstr("main")) != 0) {
    generate_type(f, fundef.ret_type);
    fprintf(f, " " SV_Fmt "(", SV_Arg(fundef.name.lexeme));
    if (fundef.args.length == 0) {
      fprintf(f, "void");
    } else {
      for (int i = 0; i < fundef.args.length; i++) {
        push_nt(&g->table, fundef.args.data[i].lexeme, NT_VAR, fun);
        if (i > 0)
          fprintf(f, ", ");
        generate_type(f, fundef.types.data[i]);
        fprintf(f, " ");
        fprintf(f, SV_Fmt, SV_Arg(fundef.args.data[i].lexeme));
      }
    }
    fprintf(f, ")\n");
    generate_compound(g, fundef.body);
    fprintf(f, "\n\n");
  } else {
    fprintf(f, "int main(int argc, char **argv) {\n");
    fprintf(f, "init_compiler_stack();\n");
    fprintf(f, "fill_cmd_args(argc, argv);\n");
    generate_compound(g, fundef.body);
    fprintf(f, "kill_compiler_stack();\n");
    fprintf(f, "return 0;\n");
    fprintf(f, "}\n\n");
  }
  end_nt_scope(&g->table);
}

int is_builtin_typename(char *name) {
  if (strcmp(name, "boolean") == 0)
    return 1;
  if (strcmp(name, "int") == 0)
    return 1;
  if (strcmp(name, "char") == 0)
    return 1;
  if (strcmp(name, "byte") == 0)
    return 1;
  if (strcmp(name, "word") == 0)
    return 1;
  if (strcmp(name, "dword") == 0)
    return 1;
  if (strcmp(name, "string") == 0)
    return 1;
  if (strcmp(name, "void") == 0)
    return 1;
  return 0;
}

void generate_forward_defs(generator_t *g, ast_t program) {
  FILE *f = g->f;
  ast_array_t stmts = program->data.program.prog;
  for (int i = 0; i < stmts.length; i++) {
    ast_t stmt = stmts.data[i];
    if (stmt->tag == tdef) {
      struct ast_tdef tdef = stmt->data.tdef;
      string_view name = tdef.name.lexeme;
      fprintf(f, "typedef struct " SV_Fmt " *" SV_Fmt ";\n", SV_Arg(name),
              SV_Arg(name));
    }
    if (stmt->tag == enum_tdef) {
      struct ast_tdef tdef = stmt->data.tdef;
      string_view name = tdef.name.lexeme;
      fprintf(f, "typedef enum " SV_Fmt " " SV_Fmt ";\n", SV_Arg(name),
              SV_Arg(name));
    }
    if (stmt->tag == embed) {
      ast_embed e = stmt->data.embed;
      if (e.is_function) {
        // Top-level embed functions - emit them here and register in name table
        generate_embed(g, stmt);

        // Register the function name from the embed block
        // Simple heuristic: look for "type name(" pattern
        if (strcmp(e.lang, "c") == 0) {
          // Extract function name from C code
          // Look for the last identifier before '('
          char *body = e.body;
          char *paren = strchr(body, '(');
          if (paren != NULL) {
            // Walk back to find the function name
            int i = paren - body - 1;

            // Skip whitespace
            while (i >= 0 && (body[i] == ' ' || body[i] == '\t')) i--;

            // Find end of identifier
            int end = i + 1;

            // Walk back to find start of identifier
            while (i >= 0 && ((body[i] >= 'a' && body[i] <= 'z') ||
                             (body[i] >= 'A' && body[i] <= 'Z') ||
                             (body[i] >= '0' && body[i] <= '9') ||
                             body[i] == '_')) {
              i--;
            }
            int start = i + 1;

            if (start < end) {
              // Register it in the name table
              string_view fname_sv = sv_from_parts(body + start, end - start);
              push_nt(&g->table, fname_sv, NT_FUN, stmt);
            }
          }
        }
      }
    }
  }

  for (int i = 0; i < stmts.length; i++) {
    ast_t stmt = stmts.data[i];
    if (stmt->tag == fundef) {
      ast_fundef fundef = stmt->data.fundef;
      if (svcmp(fundef.name.lexeme, sv_from_cstr("main")) != 0) {
        generate_type(f, fundef.ret_type);

        fprintf(f, " " SV_Fmt "(", SV_Arg(fundef.name.lexeme));
        if (fundef.args.length == 0) {
          fprintf(f, "void");
        } else {
          for (int i = 0; i < fundef.args.length; i++) {
            if (i > 0)
              fprintf(f, ", ");
            generate_type(f, fundef.types.data[i]);
            fprintf(f, " ");
            fprintf(f, SV_Fmt, SV_Arg(fundef.args.data[i].lexeme));
          }
        }
        fprintf(f, ");\n\n");
      }
    }
  }

  for (int i = 0; i < stmts.length; i++) {
    ast_t stmt = stmts.data[i];
    if (stmt->tag == tdef) {
      struct ast_tdef tdef = stmt->data.tdef;
      char *name = string_of_sv(tdef.name.lexeme);
      generate_array_funcs(g, name);
    }
  }
}

void generate_tdef(generator_t *g, ast_t tdef_ast) {
  FILE *f = g->f;
  struct ast_tdef tdef = tdef_ast->data.tdef;
  string_view name = tdef.name.lexeme;

  // Register the type in the name table so it can be looked up later
  push_nt(&g->table, name, NT_USER_TYPE, tdef_ast);

  // fprintf(f, "typedef struct %s %s;\n", name, name);
  fprintf(f, "struct " SV_Fmt "{\n", SV_Arg(name));
  if (tdef.t == TDEF_PRO) {
    fprintf(f, "enum {\n");
    for (int i = 0; i < tdef.constructors.length; i++) {
      ast_cons cons = tdef.constructors.data[i]->data.cons;
      if (i > 0)
        fprintf(f, ",\n");
      fprintf(f, SV_Fmt, SV_Arg(cons.name.lexeme));
    }
    fprintf(f, "\n} tag; \n");
    fprintf(f, "union {\n");
    for (int i = 0; i < tdef.constructors.length; i++) {
      ast_cons cons = tdef.constructors.data[i]->data.cons;
      ast_type type = cons.type->data.type;
      if (svcmp(type.name.lexeme, sv_from_cstr("void")) != 0) {
        generate_type(f, cons.type);
        fprintf(f, SV_Fmt, SV_Arg(cons.name.lexeme));
      }
    }
    fprintf(f, "} data;");
  } else {
    for (int i = 0; i < tdef.constructors.length; i++) {
      ast_cons cons = tdef.constructors.data[i]->data.cons;
      ast_type type = cons.type->data.type;
      if (svcmp(type.name.lexeme, sv_from_cstr("void")) != 0) {
        generate_type(f, cons.type);
        fprintf(f, " " SV_Fmt ";\n", SV_Arg(cons.name.lexeme));
      }
    }
  }
  fprintf(f, "};\n");

  return;
}

void transpile(generator_t *g, ast_t program) {
  FILE *f = g->f;
  ast_array_t stmts = program->data.program.prog;
  fprintf(f, "#include \"alloc.h\"\n");
  fprintf(f, "#include \"fundefs.h\"\n");
  fprintf(f, "#include \"fundefs_internal.h\"\n");
  fprintf(f, "#include \"typedefs.h\"\n\n");
  if (g->target == TARGET_ZXN)
    fprintf(f, "#define INIT_CAP_ALLOC_STACK 64\n\n");

  generate_forward_defs(g, program);

  for (int i = 0; i < stmts.length; i++) {
    ast_t stmt = stmts.data[i];
    generate_statement(g, stmt);
  }
}
