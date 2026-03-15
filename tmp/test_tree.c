#include "./src/generation/fundefs.h"
#include "./src/generation/fundefs_internal.h"
#include "./src/generation/typedefs.h"

/* Inlined allocator code */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct alloc_elem_t {
  void *ptr;
  int scope;
} alloc_elem_t;

typedef struct alloc_stack_t {
  alloc_elem_t *data;
  void **persistents;
  size_t capacity;
  size_t capacity_p;
  int length;
  int length_p;
  size_t scope;
} alloc_stack_t;

#define INIT_CAP_ALLOC_STACK 1024

void init_stack(alloc_stack_t *alloc) {
  alloc->capacity = INIT_CAP_ALLOC_STACK;
  alloc->capacity_p = INIT_CAP_ALLOC_STACK;
  alloc->data = malloc(sizeof(alloc_elem_t) * INIT_CAP_ALLOC_STACK);
  alloc->persistents = malloc(sizeof(void *) * INIT_CAP_ALLOC_STACK);
  alloc->length = 0;
  alloc->length_p = 0;
}

void kill_stack(alloc_stack_t alloc) {
  for (int i = 0; i < alloc.length; i++)
    free(alloc.data[i].ptr);
  for (int i = 0; i < alloc.length_p; i++)
    free(alloc.persistents[i]);
  free(alloc.data);
  free(alloc.persistents);
}

void push_stack(alloc_stack_t *alloc, void *ptr) {
  if (alloc->length >= (int)alloc->capacity) {
    alloc->capacity = alloc->capacity * 2;
    alloc->data = realloc(alloc->data, alloc->capacity * sizeof(alloc_elem_t));
  }
  alloc_elem_t pushed;
  pushed.ptr = ptr;
  pushed.scope = alloc->scope;
  alloc->data[alloc->length++] = pushed;
}

void push_persistent(alloc_stack_t *alloc, void *ptr) {
  if (alloc->length_p >= (int)alloc->capacity_p) {
    alloc->capacity_p = alloc->capacity_p * 2;
    alloc->persistents =
        realloc(alloc->persistents, alloc->capacity_p * sizeof(void *));
  }
  alloc->persistents[alloc->length_p++] = ptr;
}

void *pop_stack(alloc_stack_t *alloc) {
  if (alloc->length == 0)
    return NULL;
  return alloc->data[--alloc->length].ptr;
}

size_t get_top_scope(alloc_stack_t alloc) {
  if (alloc.length <= 0)
    return -1;
  return alloc.data[alloc.length - 1].scope;
}

void new_scope(alloc_stack_t *alloc) { alloc->scope++; }

void end_scope(alloc_stack_t *alloc) {
  if (alloc->length == 0)
    return;
  while (get_top_scope(*alloc) >= alloc->scope && alloc->length > 0)
    free(pop_stack(alloc));
  if (alloc->scope >= 1)
    alloc->scope--;
}

void *allocate(alloc_stack_t *alloc, size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    printf("Could not allocate\n");
    exit(1);
  }
  push_stack(alloc, ptr);
  return ptr;
}

void *allocate_persistent(alloc_stack_t *alloc, size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    printf("Could not allocate persistent\n");
    exit(1);
  }
  memset(ptr, 0, size);
  push_persistent(alloc, ptr);
  return ptr;
}

void *reallocate(alloc_stack_t *alloc, void *ptr, size_t size) {
  void *res = realloc(ptr, size);
  if (res == NULL) {
    printf("Could not reallocate data !\n");
    exit(1);
  }
  for (int i = 0; i < alloc->length; i++) {
    void *test = alloc->data[i].ptr;
    if (test == ptr) {
      alloc->data[i].ptr = res;
      break;
    }
  }
  return res;
}

void *reallocate_persistent(alloc_stack_t *alloc, void *ptr, size_t size) {
  void *res = realloc(ptr, size);
  if (res == NULL) {
    printf("Could not reallocate data !\n");
    exit(1);
  }
  for (int i = 0; i < alloc->length_p; i++) {
    void *test = alloc->persistents[i];
    if (test == ptr) {
      alloc->persistents[i] = res;
      break;
    }
  }
  return res;
}

alloc_stack_t global;

void init_compiler_stack(void) { init_stack(&global); }

void kill_compiler_stack(void) { kill_stack(global); }

void new_compiler_scope(void) { new_scope(&global); }

void end_compiler_scope(void) { end_scope(&global); }

void *allocate_compiler(size_t size) { return allocate(&global, size); }

void *allocate_compiler_persistent(size_t size) {
  return allocate_persistent(&global, size);
}

void *reallocate_compiler(void *ptr, size_t size) {
  return reallocate(&global, ptr, size);
}

void *reallocate_compiler_persistent(void *ptr, size_t size) {
  return reallocate_persistent(&global, ptr, size);
}

typedef struct tree *tree;
void print_int(int n);

void print_tabs(int n);

void print_tree_aux(tree t, int depth);

void print_tree(tree t);

__internal_dynamic_array_t tree_make_array(void) {
  return __internal_make_array(sizeof(tree), 0);
}

void tree_push_array(__internal_dynamic_array_t arr, tree elem) {
  __internal_push_array(arr, &elem);
}

tree tree_pop_array(__internal_dynamic_array_t arr) {
  tree *res = __internal_pop_array(arr);
  return *res;
}

tree tree_get_elem(__internal_dynamic_array_t arr, size_t index) {
  tree *res = __internal_get_elem(arr, index);
  if (res == NULL) {
    printf("NULL ELEMENT IN tree_get_elem");
    exit(1);
  }
  return *res;
}

void tree_set_elem(__internal_dynamic_array_t arr, size_t index, tree elem) {
  __internal_set_elem(arr, index, &elem);
}

void tree_insert(__internal_dynamic_array_t arr, size_t index, tree elem) {
  __internal_insert(arr, index, &elem);
}

struct tree {
  int Val;
  tree left;
  tree right;
};
void print_int(int n) {
  int a = n / 10;
  if (a)
    print_int(a);
  putchar(n % 10 + '0');
}

void print_tabs(int n) {
  for (int i = 1; i <= (int)n; i++) {
    print(new_string((string){.data = "|  ", .length = 3}));
  };
}

void print_tree_aux(tree t, int depth) {
  print_tabs(depth);
  print_int(t->Val);
  print(new_string((string){.data = "\n", .length = 1}));
  if (t->left != NULL)
    print_tree_aux(t->left, depth + 1);
  if (t->right != NULL)
    print_tree_aux(t->right, depth + 1);
}

void print_tree(tree t) { print_tree_aux(t, 0); }

int main(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
  {
    struct tree tmp_node1 = {.Val = 1, .left = NULL, .right = NULL};
    tree node1 = allocate_compiler_persistent(sizeof(struct tree));
    *node1 = tmp_node1;
    struct tree tmp_node2 = {.Val = 7, .left = NULL, .right = NULL};
    tree node2 = allocate_compiler_persistent(sizeof(struct tree));
    *node2 = tmp_node2;
    struct tree tmp_node3 = {.Val = 3, .left = node1, .right = node2};
    tree node3 = allocate_compiler_persistent(sizeof(struct tree));
    *node3 = tmp_node3;
    struct tree tmp_node4 = {.Val = 8, .left = NULL, .right = NULL};
    tree node4 = allocate_compiler_persistent(sizeof(struct tree));
    *node4 = tmp_node4;
    struct tree tmp_node5 = {.Val = 13, .left = NULL, .right = NULL};
    tree node5 = allocate_compiler_persistent(sizeof(struct tree));
    *node5 = tmp_node5;
    struct tree tmp_node6 = {.Val = 65, .left = node4, .right = node5};
    tree node6 = allocate_compiler_persistent(sizeof(struct tree));
    *node6 = tmp_node6;
    struct tree tmp_node7 = {.Val = 0, .left = node6, .right = node3};
    tree node7 = allocate_compiler_persistent(sizeof(struct tree));
    *node7 = tmp_node7;
    print_tree(node7);
  }
  kill_compiler_stack();
  return 0;
}
