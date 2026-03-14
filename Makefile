CC=gcc
CFLAGS=-Werror -Wall -Wextra -g -pedantic -I lib/cpu_agnostic
SRC=src/
BUILD=build/
LIB=lib/cpu_agnostic/

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

clean:
	rm -rf $(BUILD)
	rm -rf rocker.c
	rm -rf tmp_rocker
	rm -rf rockc

