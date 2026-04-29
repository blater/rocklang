CC     = gcc
CFLAGS = -Werror -Wall -Wextra -pedantic -I src
DEPFLAGS = -MMD -MP
SRC    = src/
LIB    = src/lib/
BUILD  = build/
OBJECTS = $(BUILD)alloc.o $(BUILD)ast.o $(BUILD)lexer.o $(BUILD)token.o \
          $(BUILD)parser.o $(BUILD)generator.o $(BUILD)name_table.o \
          $(BUILD)stringview.o $(BUILD)error.o $(BUILD)typechecker.o \
          $(BUILD)main.o
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean test-pools test-negative

all: $(BUILD) rockc

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)%.o: $(SRC)%.c | $(BUILD)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD)alloc.o: $(LIB)alloc.c | $(BUILD)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD)pools.o: $(LIB)pools.c $(LIB)pools.h | $(BUILD)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

rockc: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Phase A.2 — pool runtime test harness.
# Standalone C test, no Rock language integration yet.
$(BUILD)pools_test: test/pools_test.c $(BUILD)pools.o | $(BUILD)
	$(CC) $(CFLAGS) -o $@ test/pools_test.c $(BUILD)pools.o

test-pools: $(BUILD)pools_test
	$(BUILD)pools_test

# Phase B — negative tests for the typechecker's acyclicity rule (ADR §9.4).
# Each test is a Rock program that MUST fail compilation with a specific
# diagnostic.
test-negative: rockc
	test/negative/run_negative.sh

clean:
	rm -rf $(BUILD)
	rm -f rockc

-include $(DEPS)
