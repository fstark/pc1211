# PC-1211 BASIC Interpreter Makefile

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -fno-builtin -g -gdwarf-4 -fno-omit-frame-pointer -fstandalone-debug
TARGET = pc1211
SRCDIR = .
TESTDIR = tests

# Source files
SOURCES = main.c program.c tokenizer.c listing.c vm.c errors.c interactive.c line_edit.c
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d)

# Test files
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

.PHONY: all clean test help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c
	@set -e; $(CC) -MM $(CFLAGS) $< | sed 's|\($*\)\.o|\1.o $@|' > $@

clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TARGET)
	rm -rf *.dSYM
	rm -f $(DEPS)

help:
	@echo "PC-1211 BASIC Interpreter Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the interpreter (default)"
	@echo "  clean   - Remove object files and executable"
	@echo "  help    - Show this help"
	@echo ""
	@echo "Usage examples:"
	@echo "  make"
	@echo "  ./pc1211 program.bas --list"
	@echo "  ./pc1211 program.bas --dump"

# Dependencies (basic - could be auto-generated)
-include $(DEPS)
main.o: main.c opcodes.h program.h tokenizer.h listing.h vm.h errors.h
program.o: program.c program.h opcodes.h errors.h
tokenizer.o: tokenizer.c tokenizer.h program.h opcodes.h errors.h
listing.o: listing.c listing.h program.h opcodes.h errors.h
vm.o: vm.c vm.h program.h opcodes.h errors.h
errors.o: errors.c errors.h opcodes.h
interactive.o: interactive.c interactive.h errors.h vm.h program.h opcodes.h
