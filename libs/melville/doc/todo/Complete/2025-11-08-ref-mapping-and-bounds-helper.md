# Global L-Value Ref Mapping and Bounds-Safe Access Plan

## Problem Summary
- Current runtime uses `effective_index = var_base(definer) + ref`.
- However, `ref` in bytecode for a program P refers to P’s flattened `gst` (P + its ancestors), not just P’s local variables.
- When child layouts assign distinct segments per ancestor (e.g., obj=0, container=2, room=3), adding `room` base to a `ref` that actually targets `container` or `obj` can overshoot allocated storage.
- Evidence: `/world/start/void` layout ends at slot count 5, but writes compute indices 6 and 7 → heap corruption and crash.

## Evidence Highlights
- Layout for `/world/start/void`: obj→0, container→2, room→3; slot count 5.
- Logs show writes: `GLOBAL ref=3 + var_offset=3 = 6`, `GLOBAL ref=4 + var_offset=3 = 7`.
- Crash: allocator detects heap corruption; abort in `malloc` during `push()`.
- Separately, inheritance Test 1 failure stems from `write(result)` calling `syslog(arg1)` with an integer (type mismatch), not from var-base logic.

## Design Overview
- Introduce a per-program GST reference mapping.
- For each compiled program P, map `gst[i]` → `{ owner_proto, owner_local_index }`.
- At runtime for a function defined in P and a `GLOBAL ref = i`:
  1. Lookup P’s `gst[i]` mapping to determine `owner_proto` and `owner_local_index` (the index within that owner’s own var set).
  2. In the child’s code, use the flattened `ancestor_map` to get `base = ancestor_map[owner_proto].var_offset`.
  3. Compute `effective_index = base + owner_local_index`.
  4. Assert `effective_index < obj->parent->funcs->num_globals`.
- Centralize this into a global L-value helper used by `resolve_var`, `eq_oper`, and all GLOBAL_L_VALUE sites.

## Compiler Changes (compile-time mapping)
- Build and store a compact mapping array on `struct code` for the program:
  - Option A: Extend/alias existing `var_tab` (`code->gst`) if it already records owner proto and local index.
  - Option B: Add a new `struct gst_ref_map { struct proto *owner; unsigned short local_index; } *gst_map; unsigned short gst_count;` on `struct code` and fill it during `build_variable_layout()`.
- Ensure the `local_index` is the index within the owner’s own variable table, not the flattened index.
- Persist counts and allocate/free with code lifetime.

## Interpreter Changes (runtime helper)
- Add `static inline unsigned int global_index_for(struct object *obj, const struct fns *definer_fn, unsigned int ref)` that:
  - Locates the definer program (via `definer_fn->origin_proto` or the local program when NULL).
  - Reads definer program’s `gst_map[ref]` to get `{owner_proto, owner_local_index}`.
  - Uses `obj->parent->funcs->ancestor_map` to fetch `base` for `owner_proto`.
  - Computes and bounds-checks `effective_index`.
- Replace direct index math in `resolve_var`, `eq_oper`, and any other GLOBAL_L_VALUE sites with this helper.

## Safety and Diagnostics
- Add assertions and logs when:
  - `ref >= definer->gst_count`.
  - `owner_proto` not found in child’s `ancestor_map`.
  - `effective_index >= obj->parent->funcs->num_globals`.
- Compile-time: emit a one-time summary per program (count, first few mappings).
- Runtime: first-hit log per site to verify correctness, then suppress to avoid noise.

## Test/I-O Compatibility Fix
- Patch `/sys/auto::write` to coerce non-strings to strings before `syslog()` so integer results print (or adjust tests to stringify).
- This resolves the false negative in "Child cannot properly access parent's variables" where an integer value is printed as empty due to syscall rejection.

## Acceptance Criteria
- No OOB logs; no heap corruption; login path stable.
- Inheritance Test 1 prints values and passes.
- Diamond and conflict/shadowing tests remain passing.

## Implementation Steps
1. Implement compile-time GST ref mapping and attach to `struct code`.
2. Implement `global_index_for()` helper and integrate into `resolve_var`, `eq_oper`, and all GLOBAL_L_VALUE sites.
3. Add diagnostics + bounds assertions.
4. Patch `/sys/auto::write` for type coercion.
5. Rebuild; run inheritance test suite.
6. Exercise login flow (`/sys/user` → `/sys/player` → `/world/start/void`) and watch for OOB/segfault.

## Risks and Performance
- Slight memory overhead per program for `gst_map`; offsetting corruption risk is well worth it.
- Lookup is O(1) array indexing and one ancestor_map scan (or direct index if we store a compact proto→offset lookup), so performance impact should be negligible.

## Status Update (2025-11-08)

- Implemented compile-time GST ref mapping on `struct code`:
  - Added `gst_map[gst_count]` entries `{owner_proto, owner_local_index}`.
  - Populated from `gst`/`own_vars` and ancestor merge in `compile1.c`/`compile2.c`.
- Implemented centralized runtime helper `global_index_for()` with bounds checks and diagnostics.
- Refactored GLOBAL_L_VALUE access sites to use the helper:
  - `resolve_var()` in `constrct.c`.
  - `eq_oper` and `+=` paths in `oper1.c`.
  - Pre/post inc/dec and `-=` in `oper2.c`.
  - Integer-assign macros in `operdef.h` now resolve via helper.
  - `assign_to_lvalue()` in `sfun_strings.c` uses helper for globals.
  - `s_fread()` in `sys5.c` reads/writes position via helper.
- Freed `gst_map` in `free_code()` to avoid leaks.

Pending:
- Build and run inheritance tests; verify login flow stability.
- Mudlib `/sys/auto::write` coercion is deferred to mudlib (not in engine).

Next Actions:
- Rebuild from project root and run test suite.
- If any crashes/OOB occur, capture logs for quick fix.
