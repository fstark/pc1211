# PC-1211 BASIC Interpreter Makefile

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -fno-builtin -g -gdwarf-4 -fno-omit-frame-pointer -fstandalone-debug
TARGET = pc1211
SRCDIR = .
TESTDIR = tests

# Source files
SOURCES = main.c program.c tokenizer.c listing.c vm.c errors.c
OBJECTS = $(SOURCES:.c=.o)

# Test files
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

.PHONY: all clean test help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test runner
TEST_RUNNER = test_runner
TEST_RUNNER_SRC = tests/t_runner.c
TEST_RUNNER_OBJECTS = program.o tokenizer.o vm.o errors.o

$(TEST_RUNNER): $(TEST_RUNNER_SRC) $(TEST_RUNNER_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: $(TEST_RUNNER)
	./$(TEST_RUNNER)

clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TARGET) $(TEST_RUNNER)
	rm -rf *.dSYM $(TEST_RUNNER).dSYM

help:
	@echo "PC-1211 BASIC Interpreter Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the interpreter (default)"
	@echo "  clean   - Remove object files and executable"
	@echo "  test    - Run test suite (not implemented yet)"
	@echo "  help    - Show this help"
	@echo ""
	@echo "Usage examples:"
	@echo "  make"
	@echo "  ./pc1211 program.bas --list"
	@echo "  ./pc1211 program.bas --dump"

# Dependencies (basic - could be auto-generated)
main.o: main.c opcodes.h program.h tokenizer.h listing.h vm.h errors.h
program.o: program.c program.h opcodes.h errors.h
tokenizer.o: tokenizer.c tokenizer.h program.h opcodes.h errors.h
listing.o: listing.c listing.h program.h opcodes.h errors.h
vm.o: vm.c vm.h program.h opcodes.h errors.h
errors.o: errors.c errors.h opcodes.h
