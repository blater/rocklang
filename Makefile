CC     = gcc
CFLAGS = -Werror -Wall -Wextra -pedantic -I src
DEPFLAGS = -MMD -MP
SRC    = src/
LIB    = src/lib/
BUILD  = build/
OBJECTS = $(BUILD)alloc.o $(BUILD)ast.o $(BUILD)lexer.o $(BUILD)token.o \
          $(BUILD)parser.o $(BUILD)generator.o $(BUILD)name_table.o \
          $(BUILD)stringview.o $(BUILD)error.o $(BUILD)main.o
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean

all: $(BUILD) rockc

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)%.o: $(SRC)%.c | $(BUILD)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD)alloc.o: $(LIB)alloc.c | $(BUILD)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

rockc: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD)
	rm -f rockc

-include $(DEPS)
