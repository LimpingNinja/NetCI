# Plan of Action (Windsurf session pickup)

## Findings
- Ancestor map belongs on `struct code` for coherent runtime lookup and to avoid stale `parent_proto` references.
- Build is blocked by incompatible struct assignment when populating `ancestor_map`.
- `compute_var_base()` must switch from scanning direct inherits to using `self_var_offset` and the flattened `ancestor_map`.
- All GLOBAL_L_VALUE sites need a single helper applying the frame base offset with bounds checks.

## Updated Structs
- `struct code` now owns: `ancestor_map` (array of `{ proto, var_offset }`), `ancestor_count`, `ancestor_capacity`, `self_var_offset`.
- `struct ancestor_var_offset` pairs an ancestor `proto` with its absolute `var_offset` in the child’s `globals[]`.

## Function Flows
- Compile-time: `build_variable_layout()` linearizes parents, assigns absolute slots, sets `self_var_offset`, alloc/grows `ancestor_map`, writes `{ proto, var_offset }`, and stamps inherit entries.
- Runtime: `compute_var_base(obj, func)` returns `self_var_offset` for local functions; otherwise looks up the defining `proto` in `ancestor_map` and returns its absolute base.
- Global access: a centralized helper computes `abs = frame.var_offset + rel`, asserts bounds, and returns the validated pointer.

## Detailed Plan
- Unblock build by replacing the struct-copy of `merged[i]` with field-wise copies into `ancestor_map[i]`.
- Initialize new `struct code` ancestor fields at creation; add free path to avoid leaks.
- Refactor `compute_var_base()` to use `self_var_offset` and `ancestor_map` with clear logging on misses.
- Introduce a centralized GLOBAL_L_VALUE helper; refactor all sites to use it.
- Rebuild and run the inheritance test workflow; analyze logs.

## Actionable Task List
1. Fix `ancestor_map` assignment in `build_variable_layout()` to explicitly copy `proto` and `var_offset`; rerun `make`.
2. Initialize new `struct code` fields (`ancestor_map=NULL`, counts/capacity zeroed, `self_var_offset=0`) on creation; add free path on code teardown.
3. Refactor `compute_var_base()` to use `self_var_offset` and the flattened `ancestor_map`; assert/log on misses.
4. Introduce centralized global L-value helper (frame base + bounds) and refactor all GLOBAL_L_VALUE sites to call it.
5. Full rebuild from project root (`make clean && make`).
6. Run inheritance workflow script and confirm the failing test passes; review logs.

## Risks and Mitigations
- Bounds mistakes → migrate all sites to the helper; assert in helper.
- Proto/code identity mismatch → always use the active frame’s code pointer; log proto names on lookup.
- Diamond conflicts → retain consistency checks during layout; log/assert on disagreements.

## References used
- libs/melville/doc/todo/context-so-far.md (current state and architectural summary)
- Build workflow: run `make clean && make` from project root

## Summary
- Data relocation complete; population path implemented; build blocked by one incompatible assignment to `ancestor_map`.
- Next immediate action: fix the assignment loop, rebuild, then refactor `compute_var_base()` and centralize global L-value access.

# 🧠 LPC Inheritance Runtime Refactor Summary

## Developer Notes (Current Status)

### Status of Major Tasks

1. **Stabilize ancestor map data on `struct code` and ensure build succeeds – in progress**  
   The map now lives on `struct code`, and `build_variable_layout()` repopulates it. Remaining work is to replace the whole-struct assignment with explicit writes into `ancestor_map[i].proto` and `.var_offset`, then re-run `make` to confirm the toolchain builds cleanly (`src/compile1.c`).

2. **Refactor `compute_var_base()` to use ancestor map and `self_var_offset` – not started**  
   Once Step 1 builds cleanly, the interpreter helper at `src/interp.c` should:  
   (a) short-circuit to `self_var_offset` for functions defined on the child,  
   (b) walk the flattened ancestor map on the child’s code object to find the defining proto’s absolute slot, and  
   (c) raise an explicit error if the proto is missing from the map.  
   This refactor must also ensure the call frame holds the code pointer needed to reach that map.

3. **Centralize `GLOBAL_L_VALUE` helpers with bounds checks – not started**  
   Introduce a shared helper (likely in `src/interp.c` or a small utility file) that consumes the active call frame and a relative slot, applies the frame’s base offset, asserts the resulting index falls within the object’s `globals`, and returns the validated pointer. Transition `resolve_var`, arithmetic operators, and any other GLOBAL_L_VALUE sites to that helper for consistency.

4. **Rebuild and rerun inheritance tests – blocked**  
   The inheritance workflow script cannot run until the build succeeds. After compiling (`make clean && make`), execute the test harness (e.g., `test_inheritance.sh`) and verify the “Child cannot properly access parent’s variables” check passes, reviewing logs for any remaining anomalies.

---

### Task Status Overview

| Task | Status | Technical Notes |
|------|---------|-----------------|
| Extend data structures for flattened ancestor offsets | **In progress** | `struct code` now owns `ancestor_map`, `ancestor_count`, `ancestor_capacity`, and `self_var_offset`, guaranteeing each compiled program carries its proto→offset lookup alongside the bytecode and symbol tables. |
| Refactor `compute_var_base()` | Pending | Awaiting the stabilized ancestor map so runtime dispatch can use `self_var_offset` and the flattened map instead of scanning only direct inherits. |
| Centralize GLOBAL_L_VALUE helpers | Pending | Planned helper will enforce bounds and reuse the frame’s base offset for every global access site. |
| Rebuild & rerun inheritance tests | Blocked | Build must succeed before the inheritance workflow (`test_inheritance.sh`) can be executed. |

---

### Updated Struct Layout

**`struct code` anatomy (stacked top-to-bottom in memory):**

1. `num_refs`, `num_globals`, `func_list` – existing metadata for bytecode and function dispatch.  
2. `gst`, `own_vars`, `inherits` – compile-time symbol tables and direct inherit list.  
3. **New lookup tier:**  `ancestor_map` → contiguous array of `{ proto, var_offset }` pairs, `ancestor_count`, `ancestor_capacity`, and `self_var_offset` establishing the child’s base slot for its own globals.

This layout makes the flattened offsets a first-class companion of the code block, eliminating the need to hop through the proto wrapper during runtime resolution.

---

### Compiler-Facing Changes

- `build_variable_layout()` now allocates or grows the MALLOC-backed ancestor map in lockstep with the merged variable ordering. It migrates any existing entries, writes the latest proto→offset pairs, and records that child-defined globals begin at slot 0 via `self_var_offset` (`src/compile1.c`).
- The function also still stamps each inherit entry with its resolved `var_offset`, ensuring alias-based lookups remain consistent with the flattened array.

Together, these updates ensure that compile-time ordering is the single source of truth for runtime global indexing, removing the stale `parent_proto` dependency that previously caused offset drift.

---

### Why the Build Currently Fails

The loop that refreshes `ancestor_map` still copies each temporary `merged[i]` record wholesale into the destination array. Those merged records include bookkeeping fields beyond `{ proto, var_offset }`, so Clang rightfully flags the assignment as an incompatible struct copy and halts the build.

**Fix:** replace the bulk assignment with explicit field writes—`ancestor_map[i].proto = merged[i].prog; ancestor_map[i].var_offset = merged[i].var_offset;`—and re-run `make`.

---

### Actionable Next Steps for Handoff

1. **Stabilize the ancestor map:** update the loop in `build_variable_layout()` to copy proto and offset fields explicitly, then ensure `make clean && make` succeeds.
2. **Refactor `compute_var_base()`:** use the child code’s `self_var_offset` for locally defined functions and fall back to the flattened `ancestor_map` for inherited ones, raising a clear error if the defining proto is absent.
3. **Centralize GLOBAL_L_VALUE helpers:** introduce a shared helper that applies the call frame’s base offset, checks bounds against `globals`, and returns the validated pointer; convert all direct index math to this helper.
4. **Regression verification:** after compilation, run the inheritance test workflow (e.g., `test_inheritance.sh`) and confirm the “Child cannot properly access parent’s variables” scenario passes while reviewing logs for residual anomalies.

---

# Appendix: Prior Architectural Summary (What Came Before)

## Context Overview

This work addresses long-standing issues in the LPC interpreter regarding **variable base offset management**, **recursive inheritance**, and **frame-local global access resolution**. The effort began after diagnosing why the `Child cannot properly access parent's variables` test was failing — specifically, why `base_value` stayed `0` even though `base::init()` ran and assigned `100`.

The root cause traced to **incorrect global variable offset resolution** when executing parent functions in inherited contexts.

---

## 🧩 Original Problem

In the existing system:

- **Each inherited function** (e.g., from `base.c`) references globals via `GLOBAL_L_VALUE ref=N` opcodes, where `ref` is *relative to its defining program’s symbol table*.
- **At runtime**, when these functions execute in a *child object*, those globals are located at **different absolute offsets** in the flattened `globals[]` array.
- However, the interpreter accessed `globals[ref]` directly instead of `globals[var_offset + ref]`, effectively reading from the wrong slot.

Symptoms:

- Inherited variables returned `0` or undefined.  
- Assignments silently mutated unrelated memory.  
- In diamond hierarchies, values diverged between branches.  
- Eventually led to heap corruption (`msizes 8978/0 disagree`) due to out-of-bounds writes.

---

## 🚀 Correct Architecture

### Key Design Goals

1. **Per-frame `var_base`** — Each function call must know the absolute offset in the object’s `globals` array where its defining program’s variables begin.  
2. **Recursive correctness** — Works for any depth (e.g., `child → parent → grandparent`).  
3. **No global state** — Must be re-entrant, thread-safe, and immune to `longjmp` or callback re-entry.  
4. **Flattened ancestor map** — Every compiled object must carry a complete mapping of *all ancestors* and their absolute offsets.

---

## 🧱 Implementation Summary

### 1. `struct call_frame`
- Added `uint16_t var_base` (or `var_offset`).
- Set on **every function call**, not just `CALL_SUPER`.

### 2. Function origin tracking
- Added `origin_proto` to each function definition.
- Populated:
  - During proto creation (`sys5.c`).
  - During proto load from cache (`cache2.c`).
  - After `add_inherit()` finalizes (`compile1.c`).

### 3. `compute_var_base()` runtime helper
- Determines `var_offset` for any function call:
  - If the function belongs to the current program → use `self_var_offset`.
  - Else → look up the defining proto in the **flattened ancestor map**.
- Returns the absolute slot offset into `globals[]`.

### 4. Global variable access refactor
- Replaced direct `obj->globals[ref]` access with `obj->globals[frame->var_base + ref]`.
- Updated `resolve_var()` and `eq_oper()` first.
- Planned: wrap *all* global lvalue ops in centralized helpers for consistency and bounds safety.

### 5. Ancestor map introduction
- Compiler now produces a **flattened ancestor → offset** map at compile-time.
- Stored in `struct code` as:
  - `ancestor_map[]` = `{ proto, var_offset }`
  - `ancestor_count`, `ancestor_capacity`
  - `self_var_offset` = number of inherited slots (child’s own base offset)

### 6. Recursive ancestor flattening
- During `build_variable_layout()`:
  - Walk each parent’s own ancestors recursively.
  - Compute **absolute offsets** by adding parent’s `var_offset`.
  - Verify that diamonds resolve to the same slot index (sanity check).

---

## 🧭 Diagnostics Recap

Before fix: `compute_var_base()` only checked *direct* inherits.  
Failing case: `/sys/player` inherited `living` and `container`, both of which inherited `obj`.  
When calling `set_id()` (defined in `obj`), the interpreter searched `/sys/player`’s **direct** inherits, didn’t find `obj`, and defaulted to offset `0`, corrupting unrelated data.

### Recent Fixes (this session)

- Relocated the flattened ancestor lookup table to `struct code` so runtime frames can resolve `proto → var_offset` without consulting the proto wrapper.
- Extended `build_variable_layout()` to allocate/grow and repopulate `ancestor_map` with `{ proto, var_offset }`, and to set `self_var_offset` for the child’s own globals; inherit entries also stamped with absolute `var_offset`.
- Ensured function `origin_proto` is populated across creation and load paths (sys5.c, cache2.c, compile1.c `add_inherit()`), enabling correct provenance for runtime lookups.
- Adopted per-frame global base offsets at the first call sites (`resolve_var`, `eq_oper`), removing dependency on a global var base; `compute_var_base()` refactor pending to use the flattened map.
- Reduced noisy logging and added barrier-triggered layout to ensure inherits finish before variables/functions are emitted.

---

## ✅ Next Steps and Current Progress

Outlined above in the developer task list.

---

**End of Session Snapshot — Safe for Handoff / Import into Windsurf**  
*(This document includes the live developer state followed by the complete architectural narrative for historical and contextual continuity.)*
