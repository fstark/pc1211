---
name: retro-c
description: "Write C code targeting 8-bit CPUs. Use when: writing interpreter core, VM, tokenizer, program storage, expression evaluation, or any module that must be portable to an 8-bit machine. Enforces no malloc, no recursion, no stdlib beyond minimal primitives, fixed-size buffers, explicit stacks, and assembly-close idioms. Also guides separation of host (modern Unix/terminal) code from core (retargetable) code."
---

# Retro C — 8-Bit-Targetable C

Write C that compiles on a modern host but produces logic a 6502/Z80/SC61860-class CPU could run. The code isn't literally cross-compiled — it's a **reference implementation** whose control flow, memory layout, and data structures map directly to what an 8-bit assembler would produce.

## Two Worlds

Every source file belongs to exactly one world. Never mix them.

### Core (retargetable to 8-bit)

Files that implement the actual machine logic: VM execution, tokenization, program storage, expression evaluation, variable access.

**Allowed:**
- `<stdint.h>` — `uint8_t`, `uint16_t`, `int8_t`, `int16_t`
- `<stdbool.h>` — `bool`
- `<string.h>` — `memcpy`, `memmove`, `memset`, `strlen`, `strncpy`, `strcmp`, `strncmp` (these have trivial 8-bit implementations)
- `<math.h>` — only if the target hardware has a math ROM (like the PC-1211's SC61860). Otherwise, implement fixed-point.
- `<assert.h>` — for invariants that vanish in release builds
- `<setjmp.h>` — for non-local error jumps (maps to a simple PC reset on hardware)

**Forbidden:**
- `malloc`, `calloc`, `realloc`, `free` — no heap
- `stdio.h` — no `printf`, `fprintf`, `fopen` (except gated behind `#ifndef NDEBUG` for dev traces)
- Recursion — all iteration is explicit with fixed-depth stacks. Expression parsing uses precedence climbing or shunting-yard, not mutual recursion. Every "stack" is a fixed-size array with a depth counter.
- `float`/`double` arithmetic without justification (prefer `double` only when emulating a hardware FPU; otherwise use fixed-point or BCD)
- C++ vtables / dynamic dispatch — use `switch` or function pointer tables
- Variable-length arrays (VLA)
- `errno`, `signal`, `longjmp` across module boundaries
- Any POSIX or OS-specific header

**Idioms:**
- All state in a single global struct (the "machine") — program memory, variables, VM registers, stacks. One struct = one contiguous memory map, trivially mapped to fixed addresses on hardware. Do not split into multiple globals for "modularity" — that's a modern C habit that adds accessor overhead.
- Fixed-size arrays with compile-time `#define` limits
- Token streams as `uint8_t` arrays with inline data
- Pointer arithmetic for program counter navigation
- `switch` or function pointer tables for opcode dispatch (both map to jump tables in assembly)
- Explicit push/pop on fixed-depth stacks
- Sentinel/terminator values (e.g., `0x00` for end-of-line, `0xFF` for end-of-index)

### Host (modern, not retargetable)

Files that provide the development/testing shell: REPL, terminal I/O, file loading, command-line parsing, history.

**Allowed:** Anything — `malloc`, `termios`, `signal`, `getopt`, `fopen`, full `stdio.h`, POSIX APIs.

**Rule:** Host files `#include` core headers, never the reverse.

## Memory Model

One struct. One address map. Everything the machine needs lives here.

```
┌─────────────────────────────────────┐
│  Static global struct ("machine")   │
│  ┌───────────────────────────────┐  │
│  │ Program memory  (fixed buf)  │  │  uint8_t prog[PROG_MAX]
│  ├───────────────────────────────┤  │
│  │ Variables       (fixed array)│  │  VarCell vars[26]
│  ├───────────────────────────────┤  │
│  │ Array storage   (fixed array)│  │  double arr[ARRAY_MAX]
│  ├───────────────────────────────┤  │
│  │ String storage  (fixed array)│  │  char strs[26][STR_MAX+1]
│  ├───────────────────────────────┤  │
│  │ Program counter (uint8_t*)   │  │
│  ├───────────────────────────────┤  │
│  │ Expression stack (double[N]) │  │
│  ├───────────────────────────────┤  │
│  │ Call stack       (addr[N])   │  │
│  ├───────────────────────────────┤  │
│  │ FOR stack        (rec[N])    │  │
│  ├───────────────────────────────┤  │
│  │ Error state                  │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

- **One global, not two.** Do not split program storage and execution state into separate globals. On hardware it's one memory space — the C should reflect that. Splitting creates accessor functions that are just unnecessary indirection.
- **Every buffer size is a `#define` constant** reflecting the real hardware limit.
- **No buffer is ever resized at runtime.**
- Overflow → error code, not realloc.

## Data Structure Rules

1. **Inline metadata.** Line records carry their own length as a leading `uint16_t`. No external index table.
2. **Terminated sequences.** Use sentinel bytes (`0x00`, `0xFF`) instead of separate length tracking where possible.
3. **Flat layout.** No linked lists, no trees, no indirection. Arrays and offsets only.
4. **Unions for variant types.** `union { double num; char str[N]; }` with a `type` tag byte, not polymorphism.
5. **Pack data tightly.** Strings have a hard max length (e.g., 7 chars). Indexes fit in `uint8_t` or `uint16_t`.

## Control Flow Rules

1. **No recursion.** Zero. Not even bounded mutual recursion (e.g., expression parsing). Convert to iterative with explicit stacks. If you think you need recursion, you're missing a stack data structure.
2. **Bounded loops only.** Every loop must have a provable upper bound (array size, stack depth, line count).
3. **Dispatch via `switch` or function pointer table** — both produce jump tables in generated assembly. Pick one per dispatch site and stay consistent.
4. **Errors via `setjmp`/`longjmp`** to a single recovery point (simulates a hardware trap). No multi-level exception unwinding.
5. **Early return on error** in leaf functions. Macro pattern:
   ```c
   #define error_if(cond, code) do { if (cond) error_set(code); } while(0)
   ```

## Function Design

- **Short.** If a function exceeds ~40 lines, extract a helper — but the helper must not add abstraction, just reduce line count.
- **No variadic functions** in core.
- **Function pointers are fine** when they simplify dispatch (they're just indirect jumps). Prefer `const` tables of function pointers over runtime-assigned ones.
- **Parameters:** Prefer passing a pointer into the global state (e.g., `uint8_t **pc_ptr`) over copying data.
- **Return values:** Return simple scalars (`double`, `int`, `bool`). Never return structs or pointers to locals.

## Naming

- `snake_case` for everything (functions, variables, types, macros that behave like functions).
- `UPPER_CASE` for `#define` constants and macros.
- Module prefix on public symbols — no unprefixed globals or public functions.
- Private (file-scope) functions: `static`, no prefix needed.
- Types: `typedef struct { ... } TypeName;` (PascalCase for struct typedefs only).

## Error Handling

| Situation | Mechanism |
|-----------|-----------|
| Programmer bug / impossible state | `assert()` — stripped in release |
| User error at runtime (bad input, overflow) | Set error code + `longjmp` to single recovery point |
| Host-side failure (file not found) | Return error code or print message — host code only |

- **`setjmp`/`longjmp` for runtime errors.** Maps to a simple JMP to a trap address on hardware. One recovery point, not multi-level unwinding.
- **Error codes are small integers** matching the hardware's error display.
- **Never silently swallow errors.** Every code path either succeeds or triggers one of the above.
- `assert()` for invariants, not for input validation.

## Assembly Correspondence Checklist

When writing or reviewing core code, verify:

- [ ] Could each local variable live in a CPU register or on a fixed-size stack frame?
- [ ] Does the dispatch (switch or function pointer table) map to a jump table?
- [ ] Are all array accesses bounded by a compile-time constant?
- [ ] Is there zero heap allocation?
- [ ] Could every `for`/`while` loop be expressed as a counted loop with known max iterations?
- [ ] Would a 6502/Z80 programmer recognize this algorithm from the C?
- [ ] Are strings handled as fixed-length byte arrays, never as dynamic `char*`?
- [ ] Is there exactly one global state struct? No split globals, no hidden mutable file-scope statics.

## File Organization

Split files into core and host. Label each clearly.

- **Core files** implement machine logic (execution, tokenization, storage, variables). No OS dependencies.
- **Host files** provide the development shell (REPL, terminal, file I/O, CLI). Can use anything.

**Core headers never `#include` host headers.** The dependency arrow is strictly host → core.

## When This Skill Applies

Use this skill whenever:
- Writing or modifying files in the **core** category above
- Adding new opcodes, statements, or expressions to the VM
- Designing data structures for program storage or variable access
- Reviewing code for 8-bit portability
- Deciding whether new functionality belongs in core vs host

Do **not** apply these constraints to host code (interactive, line_edit, listing, main) — those files can use modern C freely.
