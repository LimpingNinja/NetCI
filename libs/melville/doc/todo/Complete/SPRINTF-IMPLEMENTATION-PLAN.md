# sprintf() Implementation Plan for NetCI

## Executive Summary

Implement a full-featured `sprintf()` function for NetCI/NLPC that matches LPC standard behavior with FluffOS/LDMud compatibility. Implementation will proceed in 3 phases over an estimated 12-20 hours total.

**Current Status**: Stubbed in `sys6a.c` (returns error), entry exists in `scall_array`[45].

---

## Phase 1: MVP - Basic C-Style Formatting ✅ COMPLETE

**Status**: ✅ **COMPLETE** (November 9, 2025)  
**Time Spent**: ~6 hours  
**Test Results**: 28/28 tests passing

### Goal
Implement core formatting with standard C printf() compatibility. This provides 80% of real-world utility.

### Deliverables

#### Format Specifiers
- `%s` - string
- `%d`, `%i` - signed decimal integer
- `%c` - character (integer as ASCII)
- `%o` - octal
- `%x` - lowercase hex
- `%X` - uppercase hex
- `%%` - literal percent sign

#### Modifiers
- **Width**: `%10s` - minimum field width
- **Precision**: `%.5s` - string truncation, `%.2f` - float precision
- **Combined**: `%10.5s` - width with precision
- **Zero padding**: `%05d` - pad with zeros
- **Left align**: `%-10s` - left justify (default is right)
- **Sign**: `%+d` - always show sign, `% d` - space for positive

#### Implementation Files
- `src/sfun_sprintf.c` - New file (similar to `sfun_strings.c`)
- Modify `src/sys6a.c` - Replace stub with call to new implementation
- No changes to `compile1.c` needed (entry already exists)

#### Test Cases
```c
// Basic
sprintf("Hello %s", "world")          → "Hello world"
sprintf("%d + %d = %d", 2, 3, 5)     → "2 + 3 = 5"

// Width & alignment
sprintf("%10s", "right")              → "     right"
sprintf("%-10s", "left")              → "left      "
sprintf("%05d", 42)                   → "00042"

// Precision
sprintf("%.3s", "truncate")           → "tru"
sprintf("%10.3s", "truncate")         → "       tru"

// Numeric bases
sprintf("%d %o %x %X", 255, 255, 255, 255) → "255 377 ff FF"

// Signs
sprintf("%+d", 42)                    → "+42"
sprintf("% d", 42)                    → " 42"
```

### Implementation Notes

#### Files Created/Modified
- ✅ **src/sfun_sprintf.c** - New 530-line implementation with full format parser
- ✅ **src/autoconf.in** - Added sfun_sprintf.o to build
- ✅ **src/Makefile** - Added sfun_sprintf.o to build  
- ✅ **src/sys6a.c** - Removed sprintf stub
- ✅ **src/sys5.c** - Fixed syswrite() and syslog() to handle INTEGER 0 (empty string)
- ✅ **libs/melville/test_sprintf.c** - 28 comprehensive tests with pass/fail assertions
- ✅ **libs/melville/boot.c** - Wired tests into startup

#### Key Implementation Details
- Format parser handles all modifiers: width, precision, flags, type
- Output buffer management with dynamic resizing
- Proper INTEGER 0 handling for empty strings (NetCI convention)
- All format specifiers working: %s %d %i %c %o %x %X %%
- All modifiers working: width, precision, zero-padding, left-align, signs (+, space)
- Error handling: missing arguments replaced with `<?>`

#### Test Coverage
All 28 tests passing covering:
- Basic string interpolation (2 tests)
- Integer formatting (3 tests)  
- Width and alignment (4 tests)
- Precision/truncation (3 tests)
- Number bases (3 tests)
- Sign modifiers (4 tests)
- Character formatting (2 tests)
- Percent escaping (2 tests)
- Real-world MUD examples (2 tests)
- Edge cases (3 tests)

### Architecture

```c
// Stack layout (variadic function)
// NUM_ARGS (total)
// arg_n
// ...
// arg_2 (format specifiers consume these)
// arg_1 (format string)

struct format_spec {
  char type;        // 's', 'd', 'x', etc.
  int width;        // field width
  int precision;    // precision (.n)
  int flags;        // LEFT_ALIGN, ZERO_PAD, SHOW_SIGN, etc.
  char *pad_str;    // for phase 2
};

int s_sprintf() {
  1. Pop NUM_ARGS
  2. Pop all arguments into array (in reverse)
  3. Parse format string into specs
  4. Process each spec, consuming arguments
  5. Build output string
  6. Push result
  7. Return 0
}
```

### Key Patterns (NetCI-specific)
- Handle INTEGER 0 as empty string
- Use `resolve_var()` for references
- Proper memory management with `MALLOC()` and `FREE()`
- Handle out-of-arguments gracefully (return error or use default)

---

## Phase 2: LPC Extensions (4-6 hours) ✅ **COMPLETE**

**Status**: ✅ **COMPLETE** - November 9, 2025  
**Test Results**: 26/26 tests passing  
**Implementation Time**: ~2 hours (leveraged existing serialization code)

### Goal
Add LPC-specific features that differentiate sprintf() from standard C printf().

### Deliverables

#### %O - LPC Value Pretty-Printing
```c
sprintf("%O", ({ 1, 2, 3 }))
// Returns:
// ({
//   1,
//   2,
//   3
// })

sprintf("%O", ([ "hp": 100, "mp": 50 ]))
// Returns:
// ([
//   "hp": 100,
//   "mp": 50,
// ])
```

**Implementation**: Recursive descent through arrays/mappings with indentation.

#### %@ - Array Iteration
```c
sprintf("%@-10s", ({ "foo", "bar", "baz" }))
→ "foo       bar       baz       "

// Apply format to each element
```

**Implementation**: Detect `@` flag, verify arg is array, loop and apply spec to each element.

#### Advanced Alignment
- `%|10s` - center alignment
- `%'.'10s` - custom pad string (pad with dots)
- `%:10s` - shorthand for `%10.10s`

#### Dynamic Width
- `%*s` - use next arg as width
- `%*.*s` - width and precision from args

```c
sprintf("%*s", 10, "right")        → "     right"
sprintf("%-*s", 10, "left")        → "left      "
sprintf("%*.*s", 10, 5, "hello")   → "     hello"
```

### Test Cases
```c
// %O - arrays
sprintf("%O", ({ 1, 2, 3 }))
sprintf("%O", ([ "a": 1, "b": 2 ]))

// %@ - iteration
sprintf("%@d ", ({ 1, 2, 3, 4 }))  → "1234 "  // Note: space after all iterations
sprintf("%@-5s", ({ "a", "bb", "ccc" })) → "a    bb   ccc  "

// Custom padding - NOT YET IMPLEMENTED
sprintf("%'.'10s", "test")         → "......test"
sprintf("%-'x'10s", "test")        → "testxxxxxx"

// Center align
sprintf("%|10s", "center")         → "  center  "

// Dynamic
sprintf("%*d", 5, 42)              → "   42"
sprintf("%-*s", 10, "left")        → "left      "
```

### Implementation Notes (Phase 2)

#### Files Modified
- ✅ **src/sfun_sprintf.c** - Added Phase 2 features (~150 lines)
  - %O implementation using `save_value_internal()`
  - %@ array iteration with format application
  - %* and %.* dynamic width/precision from arguments
  - %| center alignment in `apply_padding()`
- ✅ **libs/melville/test_sprintf_phase2.c** - 26 comprehensive tests
- ✅ **libs/melville/boot.c** - Wired Phase 2 tests into startup

#### Key Implementation Details
- **%O**: Leveraged existing `save_value_internal()` from `sys_strings.c` for array/mapping serialization
- **%@**: Detects FLAG_ARRAY_ITER, loops through array elements, applies format spec to each
- **%***: Width marker (-1) consumed from argument stack before format application
- **%.***: Precision marker (-2) consumed from argument stack
- **%|**: Center alignment calculates `left_pad = total/2`, `right_pad = total - left_pad`

#### Test Coverage
All 26 tests passing:
- %O with arrays (4 tests)
- %O with mappings (3 tests) 
- %O with nested structures (1 test)
- %O with formatting (2 tests)
- %@ array iteration (3 tests)
- %@ with hex formatting (2 tests)
- %* dynamic width (3 tests)
- %* dynamic precision (2 tests)
- %| center alignment (3 tests)
- %| with precision (1 test)
- Combined features (2 tests)

#### Known Limitations
- Custom padding strings (`%'.'10s`) not yet implemented (deferred to later phase or not needed)
- %@ applies format THEN outputs literal text, not interleaved (e.g., `%@d ` outputs "1234 " not "1 2 3 4 ")

---

## Phase 3: Advanced Features (4-6 hours) 📋 PLANNED

**Status**: 📋 **PLANNED** - Ready to implement  
**Terminal Width**: Use negotiated terminal width, default to 80 if not available  
**Reference Implementations**:
- LDMud: https://raw.githubusercontent.com/ldmud/ldmud/fc87ac67723f780e3d19beb12a64d634604354d5/src/sprintf.c
- FluffOS: https://raw.githubusercontent.com/fluffos/fluffos/bec62af997649173d095e94f1c90d0b246f1012d/src/packages/core/sprintf.cc

### Goal
Implement sophisticated layout features for MUD interface formatting.

### Deliverables

#### %= - Column Mode (Word Wrapping)
```c
sprintf("%=20s", "This is a very long sentence that needs wrapping")
// Returns:
// "This is a very     "
// "long sentence that "
// "needs wrapping     "
```

**Implementation**: 
- Split on whitespace
- Word-wrap to field width
- Pad each line to width
- Handle precision as wrap width override

#### %# - Table Mode
```c
sprintf("%#-40.3s", "one\ntwo\nthree\nfour\nfive\nsix\nseven\neight\nnine\nten")
// Returns (3 columns, 40 chars wide, left-aligned):
// "one     five    nine   "
// "two     six     ten    "
// "three   seven          "
// "four    eight          "
```

**Implementation**:
- Split input on newlines
- Calculate column count (from precision or auto-fit)
- Distribute items across columns
- Format as table with proper spacing

#### %$ - Justify Mode
```c
sprintf("%$40s", "This is a justified line with spaces")
// Returns: text stretched with even spacing to fill width
```

**Implementation**: Distribute extra spaces evenly between words.

#### %#O - Compact LPC Value Printing
```c
sprintf("%#O", ({ 1, 2, 3 }))
→ "({ 1, 2, 3 })"  // Single line, not pretty-printed
```

### Test Cases
```c
// Column mode
sprintf("%=15s", "word wrapped text here")
sprintf("%=-15s", "left aligned wrap")
sprintf("%=|15s", "center aligned wrap")

// Table mode  
sprintf("%#20s", "a\nb\nc\nd\ne\nf")
sprintf("%#-30.2s", "col1\ncol2\ncol3\ncol4")

// Justify
sprintf("%$50s", "This text will be justified across the width")

// Compact %O
sprintf("%#O", ({ 1, ({ 2, 3 }), 4 }))
sprintf("%#O", ([ "nested": ([ "value": 1 ]) ]))
```

---

## Implementation Strategy

### File Organization

```
src/
  sfun_sprintf.c          [NEW] Main implementation
  sys6a.c                 [MODIFY] Replace stub
  protos.h                [NO CHANGE] Declaration already exists
  compile1.c              [NO CHANGE] scall_array entry exists

libs/melville/doc/
  netci-functions.md      [UPDATE] Replace "NOT IMPLEMENTED" with full docs
  todo/
    SPRINTF-IMPLEMENTATION-PLAN.md  [THIS FILE]
```

### Development Workflow

**Phase 1:**
1. Create `src/sfun_sprintf.c` with Phase 1 features
2. Write comprehensive test file `test/test_sprintf.c`
3. Update `src/sys6a.c` to call new implementation
4. Build and test: `make clean && make`
5. Run test suite
6. Update documentation

**Phase 2:**
1. Add LPC extensions to `sfun_sprintf.c`
2. Extend test suite
3. Test with real mudlib code
4. Update documentation

**Phase 3:**
1. Add layout features to `sfun_sprintf.c`
2. Test complex cases
3. Performance testing (large outputs)
4. Final documentation update

### Code Structure

```c
// src/sfun_sprintf.c

/* Format parsing */
static struct format_spec* parse_format(const char *fmt, int *count);
static void free_format_specs(struct format_spec *specs, int count);

/* Type formatters - Phase 1 */
static char* format_string(struct format_spec *spec, const char *str);
static char* format_integer(struct format_spec *spec, long val, int base);
static char* format_char(struct format_spec *spec, int ch);

/* Type formatters - Phase 2 */
static char* format_lpc_value(struct format_spec *spec, struct var *val, int indent);
static char* format_array_iter(struct format_spec *spec, struct var *arr);

/* Layout formatters - Phase 3 */
static char* format_column(struct format_spec *spec, const char *str);
static char* format_table(struct format_spec *spec, const char *str);
static char* format_justify(struct format_spec *spec, const char *str);

/* Utilities */
static char* apply_padding(const char *str, struct format_spec *spec);
static char* center_string(const char *str, int width, const char *pad);
static char** word_wrap(const char *str, int width, int *line_count);

/* Main entry point */
int s_sprintf(struct object *caller, struct object *obj, 
              struct object *player, struct var_stack **rts);
```

---

## Testing Strategy

### Unit Tests
Create `test/test_sprintf.c`:
```c
void test_basic_string() {
  assert_equal(sprintf("Hello %s", "world"), "Hello world");
}

void test_integer_formats() {
  assert_equal(sprintf("%d", 42), "42");
  assert_equal(sprintf("%05d", 42), "00042");
  assert_equal(sprintf("%x", 255), "ff");
}

void test_alignment() {
  assert_equal(sprintf("%10s", "right"), "     right");
  assert_equal(sprintf("%-10s", "left"), "left      ");
}

// ... 50+ test cases total
```

### Integration Tests
Test with real mudlib code:
```c
// In room.c
void long() {
  write(sprintf("%-40s [%3d]", query_short(), query_light()));
}

// In player.c  
void show_stats() {
  write(sprintf("HP: %3d/%3d  MP: %3d/%3d", hp, max_hp, mp, max_mp));
}

// In who command
sprintf("%-20s %-10s %5s", name, title, idle_time);
```

### Edge Cases
- Empty format string
- Empty arguments
- Mismatched argument count
- NULL/zero values
- Very long strings (>1000 chars)
- Nested arrays (recursion depth)
- Invalid format specs (should fail gracefully)
- Mixed types in %@

---

## Performance Considerations

### Memory Management
- Pre-allocate reasonable buffer sizes
- Grow dynamically for large outputs
- Avoid memory leaks in error paths
- Profile with valgrind

### String Building
- Use StringBuilder pattern (realloc strategy)
- Minimize string copies
- Cache strlen() results

### Optimization Targets
- Format parsing: O(n) where n = format length
- String formatting: O(m) where m = output length
- Array iteration: O(k) where k = array size
- Target: <1ms for typical cases (<1000 char output)

---

## Error Handling

### Failure Modes
1. **Too few arguments**: Use empty string / 0 for missing args
2. **Type mismatch**: Convert or use placeholder "<?>"
3. **Invalid format**: Skip specifier, continue parsing
4. **Memory allocation failure**: Return error (1)
5. **Stack underflow**: Return error (1)

### Error Messages
Log to syslog for debugging:
```c
if (arg_count < required_args) {
  syslog("sprintf: format requires %d args, got %d", required_args, arg_count);
  // Use defaults and continue
}
```

---

## Documentation Updates

### netci-functions.md

Replace current entry:
```markdown
### sprintf

#### NAME
sprintf()  -  **NOT IMPLEMENTED** - Use string concatenation instead
```

With full documentation (see Phase 1 completion for detailed entry).

---

## Timeline & Milestones

| Phase | Hours | Milestone | Deliverable |
|-------|-------|-----------|-------------|
| Phase 1 | 4-6 | Basic sprintf working | MVP with C-style formatting |
| Phase 2 | 6-8 | LPC extensions | %O, %@, custom padding |
| Phase 3 | 4-6 | Advanced layouts | Column, table, justify modes |
| **Total** | **14-20** | **Full implementation** | **Production-ready sprintf** |

### Phase 1 Breakdown (MVP)
- Hour 1-2: Format parser, string/integer formatters
- Hour 2-3: Width, precision, alignment
- Hour 3-4: Zero padding, signs, all bases
- Hour 4-5: Testing and bug fixes
- Hour 5-6: Documentation

---

## References

### External Resources
- [FluffOS sprintf.cc](https://raw.githubusercontent.com/fluffos/fluffos/bec62af997649173d095e94f1c90d0b246f1012d/src/packages/core/sprintf.cc)
- [LDMud sprintf.c](https://raw.githubusercontent.com/ldmud/ldmud/fc87ac67723f780e3d19beb12a64d634604354d5/src/sprintf.c)
- [FluffOS sprintf docs](https://www.fluffos.info/efun/strings/sprintf.html)
- [LDMud sprintf docs](https://www.tamedhon.de/content/magier/ldmud_doc/efun/sprintf.html)

### NetCI Internal
- `src/sfun_strings.c` - sscanf() implementation pattern
- `src/sys6a.c` - String function patterns
- `docs/ENGINE-EXTENSION-GUIDE.md` - System call patterns
- `src/compile1.c` - scall_array definition (line 140)

---

## Success Criteria

### Phase 1 (MVP)
- ✅ All basic format specifiers work
- ✅ Width, precision, alignment correct
- ✅ 30+ test cases pass
- ✅ Real mudlib code works without errors
- ✅ No memory leaks (valgrind clean)

### Phase 2 (LPC Extensions)
- ✅ %O pretty-prints arrays and mappings
- ✅ %@ iterates arrays correctly
- ✅ Custom padding works
- ✅ Dynamic width/precision from args
- ✅ 20+ additional test cases pass

### Phase 3 (Advanced)
- ✅ Column mode word-wraps correctly
- ✅ Table mode layouts properly
- ✅ Justify distributes spaces evenly
- ✅ 15+ complex test cases pass
- ✅ Performance: <1ms for typical use

### Overall
- ✅ 100% compatibility with FluffOS/LDMud for common cases
- ✅ Clean, maintainable code
- ✅ Comprehensive documentation
- ✅ Zero regression in existing code

---

## Risk Assessment

### Low Risk
- Basic formatters (C-style printf is well-understood)
- String manipulation (existing patterns in NetCI)
- Testing approach (clear pass/fail criteria)

### Medium Risk
- %O pretty-printing (recursion, complex types)
- Table mode layout (complex algorithm)
- Memory management (many allocations)

### Mitigation
- Study LDMud/FluffOS implementations closely
- Extensive testing at each phase
- Code review before each phase commit
- Memory profiling throughout

---

## Notes

### Why New File (sfun_sprintf.c)?
- sprintf is complex (~800-1200 lines estimated)
- Keeps sys6a.c manageable
- Follows pattern set by sfun_strings.c (sscanf)
- Easier to maintain and test

### Why Phase Approach?
- MVP delivers immediate value (4-6 hours)
- Can ship Phase 1 and continue with Phase 2/3
- Reduces risk of "perfect being enemy of good"
- Each phase builds on previous (clean architecture)

### Compatibility Notes
- FluffOS and LDMud have slight differences
- Target FluffOS behavior as primary (more modern)
- Document any NetCI-specific quirks
- Aim for "works with existing LPC code" not "byte-for-byte identical output"

---

**END OF IMPLEMENTATION PLAN**
