CC=gcc
CFLAGS=-Werror -Wall -Wextra -pedantic -I src
SRC=src/
BUILD=build/
LIB=src/lib/

.PHONY: all create_build clean

all: create_build rockc

create_build:
	@mkdir -p $(BUILD)

$(BUILD)%.o: $(SRC)%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)alloc.o: $(LIB)alloc.c
	$(CC) $(CFLAGS) -c $< -o $@

rockc: $(BUILD)alloc.o $(BUILD)ast.o $(BUILD)lexer.o $(BUILD)token.o $(BUILD)parser.o $(BUILD)generator.o $(BUILD)name_table.o $(BUILD)stringview.o $(BUILD)error.o $(BUILD)main.o
	$(CC) $(CFLAGS) -o $@ $^


# ---------------------------------------

# Rock program build targets
ROCK_LIB_SRCS = src/lib/alloc.c src/lib/fundefs.c \
                src/lib/fundefs_internal.c src/lib/asm_interop.c

# Build a Rock exe for local cc
# Usage: make target-cc SRC=path/to/foo.rkr OUT=path/to/foo
.PHONY: target-cc
target-cc: rockc
	./rockc $(SRC) $(OUT)
	$(CC) -Wall -I $(CURDIR)/src \
	    -o $(abspath $(OUT)) $(abspath $(OUT)).c \
	    $(addprefix $(CURDIR)/,$(ROCK_LIB_SRCS))

# Build a Rock exe for ZXN Spectrum Next (zcc)
# Usage: make target-zxn SRC=path/to/foo.rkr OUT=path/to/foo
.PHONY: target-zxn
target-zxn: rockc
	./rockc $(SRC) $(OUT) --target=zxn
	zcc +zxn \
	    -subtype=nex \
	    -startup=1 \
	    -clib=sdcc_iy \
	    "-pragma-include:$(CURDIR)/src/lib/zxn/zpragma_zxn.inc" \
	    --opt-code-size -SO3 \
	    --max-allocs-per-node200000 \
	    -create-app \
	    -I $(CURDIR)/src \
	    -o $(abspath $(OUT)) $(abspath $(OUT)).c \
	    $(addprefix $(CURDIR)/,$(ROCK_LIB_SRCS))

clean:
	rm -rf $(BUILD)
	rm -f rocker.c
	rm -rf tmp_rocker
	rm -f rockc

