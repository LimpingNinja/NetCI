
How does LPC handle recursive inheritance ::create function calls

###### ChatGPT said:

Good question — this is one of the trickier corners of LPC’s object model.

Here’s how recursive inheritance and `::create()` calls are handled in most modern LPC drivers (LDMud, FluffOS, MudOS):

----------

### 🔹 1. Inheritance Tree Evaluation

When an object `A` inherits another object `B`, the driver loads `B` first (if not already loaded), then `A`.  
If `B` itself inherits something, this continues recursively until the entire inheritance graph is resolved.  
Then, the driver flattens that graph into a **single linear inheritance chain**, in _reverse dependency order_:  
base → mid → top (so that base classes are initialized first).

----------

### 🔹 2. `::create()` Propagation

Each object can define a `create()` function, which acts as a constructor called after compilation and initialization.

-   When `A` inherits `B`, and both have a `create()`, the driver ensures both are called _exactly once_, from bottom to top.
    
-   The **child’s `create()`** runs last (most-derived), and it can manually invoke its parent’s one with `::create()`.
    
-   If the driver is performing object creation (e.g., `clone_object()`), it executes the **create() of the most derived object**, and within that, each `::create()` can optionally call up the chain.
    

So it’s **not automatic recursion**: `::create()` only recurses if the developer explicitly chains them.

----------

### 🔹 3. Recursive or Cyclic Inheritance

If object A inherits B and B inherits A (directly or indirectly), drivers detect the cycle during compile-time and error out. There’s no “infinite recursion” on `::create()` because the compiler will reject circular dependency graphs.

However, if you manually call `::create()` within `create()` without guards, you can accidentally cause recursion by re-cloning or reloading objects that trigger their own constructors.

Example of a safe chain:

`inherit "/std/base"; void  create() {
    ::create(); // calls base's create() setup(); // child setup }` 

But this would be dangerous:

`void  create() {
    clone_object(file_name(this_object())); // triggers itself }` 

----------

### 🔹 4. Multiple Inheritance

If multiple inherited parents have `create()`, their order is the order in which they are listed in the `inherit` statements, unless the driver linearizes differently (FluffOS and LDMud preserve declaration order).  
You can explicitly call `Parent1::create()` and `Parent2::create()` to handle initialization selectively.

----------

### 🔹 5. Summary Table

Scenario

Who calls `create()`

Order

Notes

Single inheritance

Child calls `::create()`

Base → Child

Safe, standard

Multi inheritance

Explicit call to each parent

Declared order

Manual control

No `::create()` call

Only child runs

–

Base not initialized

Recursive inheritance

Compile error

–

Prevented by driver

Manual recursion

Developer’s fault

Runtime loop

Use guards

----------

If you want, I can show you **the actual call trace from LDMud’s `object.c`** (where it resolves the create chain) so you can see exactly when and how the interpreter steps through `::create()` — would you like that?

##### You said:

Does LPC copy all variables from the inherited objects

###### ChatGPT said:

Thought

Thought for 26s

Short answer: yes—**all non-private variables from inherited programs become part of your object’s own variable set**. But they’re not “copied from a parent instance”; LPC flattens inheritance at compile time.

Here’s the nuance:

-   **Compile-time flattening:** When you `inherit` a program, its variables are merged into the child’s program layout. Each clone of the child gets its **own storage slots** for those inherited globals. There’s no separate parent object holding state. [FluffOS](https://www.fluffos.info/lpc/constructs/inherit.html?utm_source=chatgpt.com)
    
-   **Visibility rules apply:** Variables marked `private` in an ancestor exist in the final object but **aren’t accessible** from the child; you can’t weaken visibility (e.g., make a private var public) in the child. You _can_ apply modifiers to inherited members en masse (e.g., `nosave`), within driver rules. [Ldmud](https://www.ldmud.eu/lpc-inheritance.html?utm_source=chatgpt.com)
    
-   **Name conflicts:** If two parents define the same variable name, it’s an error/ambiguous in most mudlibs—you must rename or restructure. (Unlike functions, there’s no `Parent::var` qualification.)
    
-   **Multiple/diamond inheritance:** If the same base is inherited along two paths, use the mudlib’s _virtual inheritance_ pattern (where available) so there’s only **one shared set** of that base’s variables in the final object. [Geas](https://www.geas.de/tutorial/lpc_57.html?utm_source=chatgpt.com)
    
-   **Persistence:** `save_object()` will serialize **inherited variables too** (unless `nosave`). That’s a direct consequence of everything being part of one object’s variable table. (Behavior follows from the compile-time merge above.) [FluffOS+1](https://www.fluffos.info/lpc/constructs/inherit.html?utm_source=chatgpt.com)
    

If you want, I can sketch a tiny example showing a parent with `private`/`public` vars, two children (one with multiple inheritance), and the resulting access/errors.

![](https://www.google.com/s2/favicons?domain=https://www.geas.de&sz=32)

![](https://www.google.com/s2/favicons?domain=https://www.ldmud.eu&sz=32)

![](https://www.google.com/s2/favicons?domain=https://www.fluffos.info&sz=32)

Sources

##### You said:

Nice, this is great and they only point to the proto objects and copy functionality and not new Compiles attached to the object? How do they handle parent::fundtion() when that function changes some global scope variable in the child?

###### ChatGPT said:

Thought for 36s

Great follow-ups. Here’s the precise model most LPC drivers use (LDMud/FluffOS/MudOS):

### Programs vs. objects (no “live parent instance”)

-   When you `inherit` a file, the **child’s program** is compiled with the parent’s **code and variable layout merged in** (flattened).
    
-   **Objects (clones)** point to a **program** (the “proto/code”), and carry **their own variable storage** (one contiguous table that includes inherited vars).
    
-   There is **no separate, running parent object** whose state you’re sharing. You’re sharing **code**, not **state**.
    

### What happens on `Parent::function()`?

-   The call runs **in the same object** (`this_object()` is the child/most-derived object).
    
-   Any global the parent function touches refers to **that object’s variable slot** for the parent’s variable. So mutations are seen by the child because it’s literally the same storage.
    
-   If a variable is `private` in the parent, the child can’t _name_ it in child code, but the parent’s code can still read/write it (it’s the same storage).
    
-   You **cannot** define a child variable with the same name as a parent variable to “override” it; that’s a compile error in mainstream drivers. So there’s no ambiguity: the parent writes its own var, which lives in your object.
    

### Can parent code mutate “child-only” globals?

-   No. Parent code can’t see identifiers that only exist in the child, because names are resolved at compile time inside the parent’s program unit.
    
-   But the child can of course call the parent function and then act on its own vars, or pass data by parameter/closure.
    

### About “only pointing to proto objects” and recompiles

-   Each object points to a compiled **program**; many objects can share it.
    
-   If you **recompile the parent file**, the driver loads a new **program** for that file. Already-loaded children **don’t automatically get rebuilt**; they still reference the **old** flattened layout they were compiled with.
    
-   To pick up parent changes, you usually need to **recompile/reload the child** (or destruct and recreate) so its program is rebuilt against the new parent program layout.
    
-   There are exceptions/tunables in some mudlibs (e.g., auto-reload daemons), but the safe mental model is: **dependencies aren’t hot-swapped; recompile dependents**.
    

### Quick example

`// parent.c  int p; // parent global private int hid; // parent private  void  parent_fn() {
    p += 1; // mutates the child's storage of p hid = 42; // also mutates storage; child can't name 'hid' }` 

`// child.c inherit "/path/parent.c"; int q; // child-only var  void  create() {
    Parent::parent_fn(); // or ::parent_fn() depending on style  // p was incremented; we can read p here.  // q is child-only; parent_fn() never touched it directly. }` 

### TL;DR

-   Inheritance flattens at compile time: **one object, one variable table, shared code**.
    
-   `Parent::function()` runs **in the child object** and mutates the **same** globals defined by the parent.
    
-   Parent code can’t see child-only vars.
    
-   Changing a parent file doesn’t retroactively change already-compiled children; **reload/recompile** them to pick up layout/code changes.
    

If you want, I can show a minimal driver-agnostic demo you can drop into LDMud/FluffOS to print offsets and prove the single storage table in action.

##### You said:

Got it I’m not sure I understand if you flatten them all into one source code then how do you reference the actual function for each? I’m a bit confused

###### ChatGPT said:

Thought for 9s

Totally fair confusion. “Flattened” doesn’t mean the compiler throws away who-came-from-where. Here’s the mental model that usually helps:

### What the compiler builds

-   **One program image** for the child, but inside it the driver keeps:
    
    -   a **function table** (every function gets a slot/index);
        
    -   an **inherit table** (each inherited file = an entry with mappings into the child’s tables);
        
    -   a **variable table** (same idea for vars).
        
-   Each function table entry carries **flags + origin info** (which inherited program it came from, visibility, etc.). So identity isn’t lost.
    

### How calls are resolved

-   **Unqualified call** `foo()`  
    Resolved at compile time to the **most-derived** definition in your linearized MRO (child first, then ancestors).
    
-   **Qualified call** `Parent::foo()`  
    The compiler looks up `Parent` in the **inherit table**, then picks that program’s `foo` and emits a direct call to that **specific entry** (by index) in the merged function table. There’s no separate parent object at runtime—the call just jumps to the correct code block inside the single program image.
    
-   **“Super” call** `::foo()` (where supported)  
    This means “call the **next** definition up the chain,” i.e., bypass your override and invoke the inherited one immediately above you in the linearized order.
    

### Why this works even though it’s “flattened”

“Flattened” only refers to **layout and code sharing**. The driver still tracks **origin metadata** so it can:

-   choose the right body for `Parent::foo()` vs plain `foo()`;
    
-   enforce visibility (`private`, `protected`);
    
-   detect conflicts (two parents define `foo` → you must qualify).
    

### Function pointers/closures

-   A closure like `(: foo :)` is compiled to a **function index** in the current program plus the bound object. If `foo` was inherited, it’s still just an index to that entry; the origin is known from metadata.
    
-   You can also take closures to qualified functions in drivers that allow it, e.g. `(: Parent::foo :)`.
    

### Inspecting what got picked

-   In many drivers, `function_exists("foo", this_object(), 1)` (note the flag) will tell you **which program** actually defines the resolved `foo`. Handy for sanity checks.
    

----------

#### Tiny example

`// base.c  void  hello() { write("base\n"); } // mid.c inherit "/base.c"; void  hello() { ::hello(); write("mid\n"); } // super-call to base  // top.c inherit "/mid.c"; void  hello() { mid::hello(); write("top\n"); } // explicit qualify to mid` 

-   `hello()` unqualified in `top.c` would pick `top`’s version.
    
-   `mid::hello()` forces the mid-level body (which itself calls `::hello()` i.e., base).
    
-   At runtime there’s **one object**; calls are just jumps to the right function **indices** tracked by the inherit/function tables.
    

So: the compiler **merges** everything into one program image but **retains origin mappings**. Qualified calls use those mappings to hit the exact ancestor’s function body; unqualified calls use normal override rules.

Messages beyond this point are only visible to you

##### You said:

Give me a suggestion on changing a recursive descent, here is an analysis of our issues: Analysis: Where We Went Wrong with LPC Inheritance The Core Problem We implemented automatic recursive inheritance initialization when we should have implemented explicit manual chaining. This is a fundamental misunderstanding of how LPC works. What We Did Wrong 1. Misunderstood ::function() Semantics What we implemented: ::init() automatically searches for and calls the parent's init() We traverse the inheritance chain looking for parent functions We try to be "smart" about which parent to call What LPC actually does: ::function() is a manual, explicit call to the immediate parent's version It's syntactic sugar for "call the version of this function that my parent defined" There is NO automatic traversal or searching The developer controls the chain by explicitly calling ::create() in their create() 2. Wrong Approach to Named Parent Calls What we implemented: parent_name::function() searches through the inheritance list at runtime We look up the parent proto by basename matching We try to find which proto the current function belongs to What LPC actually does: parent_name::function() is resolved at compile time The compiler knows exactly which parent you're referring to from the inherit statement It's a direct function pointer, not a runtime lookup The syntax is for disambiguation in multiple inheritance, not dynamic dispatch 3. Confused Proto Reuse What we did: Initially tried to reuse protos across compilations Then switched to always compiling fresh This causes massive memory waste and compilation overhead What LPC actually does: Protos are compiled once and cached Multiple objects inherit from the same proto The proto is immutable once compiled Function pointers in the proto point to that proto's code, period The Correct LPC Model Compilation Phase 1. Parse "inherit /std/base" 2. Check if /std/base is already compiled - YES: Reuse the existing proto (it's immutable) - NO: Compile it now, cache it 3. Add a reference to that proto in current file's inherit list 4. Copy parent's global variables to child's symbol table 5. Continue compiling child Function Resolution at Compile Time When the compiler sees ::create(): 1. Look at the CURRENT file's inherit list 2. Take the FIRST parent (or the specific named parent) 3. Generate a DIRECT CALL to that parent's function 4. Store the function pointer in the bytecode When the compiler sees base::create(): 1. Look through the inherit list for an inherit with basename "base" 2. Find that parent proto 3. Look up "create" in THAT proto's function table 4. Generate a DIRECT CALL with that function pointer 5. Store it in the bytecode Runtime Execution 1. Object is cloned 2. Driver calls the object's create() function (if it exists) 3. Inside create(), if there's ::create(), it calls the parent's create() 4. This is a DIRECT function call, not a lookup 5. No searching, no traversal, no "finding which proto" Specific Bugs in Our Implementation Bug #1: find_parent_function and find_named_parent_function Problem: These functions do runtime lookup and traversal. Why it's wrong: In LPC, parent calls are resolved at compile time The bytecode contains a direct function pointer There's no "searching" at runtime What should happen: Compiler generates CALL_PARENT opcode with a function pointer Runtime just executes that pointer No lookup, no proto traversal Bug #2: Proto Creation in add_inherit Problem: We're compiling the parent fresh every time it's inherited. Why it's wrong: If 10 files inherit from /std/base, we compile it 10 times Each gets its own proto with its own function table Massive memory waste Function pointers are different for the "same" function What should happen: Compile /std/base once Cache the proto globally All 10 files reference the SAME proto Function pointers are identical Bug #3: Current Function Context Tracking Problem: We pass current_func around to figure out which proto we're in. Why it's wrong: This is runtime overhead for something that should be compile-time We're trying to solve a problem that doesn't exist in proper LPC What should happen: Compiler knows which function is calling which parent It's encoded in the bytecode Runtime doesn't need to figure anything out The Right Architecture Compile Time c // When we see: inherit "/std/base"; 1. Check global proto cache for "/std/base" 2. If not found: - Compile /std/base - Create proto - Add to global cache 3. Add proto reference to current file's inherit list 4. Copy parent globals to child // When we see: ::create() 1. Look at current file's first inherit 2. Look up "create" in that proto's function table 3. Emit: CALL_PARENT_FUNC <function_pointer> // When we see: base::create() 1. Find inherit with basename "base" in current file 2. Look up "create" in that proto's function table 3. Emit: CALL_NAMED_PARENT <function_pointer> Runtime c // CALL_PARENT_FUNC opcode case CALL_PARENT_FUNC: func_ptr = code[pc++].value.func_ptr; call_function(func_ptr, obj); break; // CALL_NAMED_PARENT opcode case CALL_NAMED_PARENT: func_ptr = code[pc++].value.func_ptr; call_function(func_ptr, obj); break; No searching. No traversal. Just direct calls. Why Our Current Code Fails The player.c Error When compiling player.c : 1. inherit LIVING_PATH - Compiles living.c fresh - living.c inherits OBJECT_PATH - Compiles object.c fresh - Returns 2. inherit CONTAINER_PATH - Compiles container.c fresh - container.c inherits OBJECT_PATH - Compiles object.c AGAIN (fresh) - This might corrupt tokenizer state - Returns with wrong token position 3. Parser expects semicolon, gets garbage The Infinite Recursion When room.c calls container::init(): We search for "container" in room's inherits ✓ We find container's proto ✓ We look up "init" in container's proto BUG: We get a function pointer, but we don't know if it's the right one Because we compiled container.c multiple times, there are multiple "init" functions We might get room's init, or container's init, or object's init If we get room's init → infinite recursion The Path Forward Phase 1: Global Proto Cache Create a global hash table: pathname → proto Modify add_inherit to check cache first Only compile if not cached Reuse cached protos Phase 2: Compile-Time Function Resolution When parsing ::function(), look up the function in parent proto Store the function pointer in the bytecode (not the name) Emit CALL_PARENT_FUNC with that pointer Phase 3: Simplify Runtime Remove find_parent_function and find_named_parent_function PARENT_CALL and NAMED_PARENT_CALL just execute the stored function pointer No searching, no traversal Phase 4: Fix Tokenizer State Ensure parse_code doesn't corrupt parent's tokenizer Each compilation should have isolated token state Or save/restore token position around recursive compiles Summary We went wrong by: Making parent calls a runtime operation instead of compile-time Compiling parents multiple times instead of caching Searching for functions at runtime instead of storing pointers at compile time Trying to be "smart" about inheritance instead of following LPC's explicit model The fix requires: Global proto cache Compile-time function pointer resolution Simplified runtime with direct calls Proper tokenizer state isolation This is a significant refactor, but it's the correct LPC model.

###### ChatGPT said:

That’s an excellent analysis — you’ve essentially reverse-engineered how _proper_ LPC compilers handle inheritance, and spotted exactly where your runtime recursive descent went off the rails.

Here’s a practical **refactor plan** to move from your recursive-descent “smart” system to an LPC-style compile-time architecture without rewriting everything from scratch.

----------

### 🧭 High-Level Strategy

You need to turn your **recursive, runtime-driven inheritance traversal** into a **two-phase compile pipeline**:

1.  **Phase 1: Proto Resolution and Caching**
    
    -   Build and cache immutable “proto” (compiled parent programs) once.
        
    -   Create a global cache keyed by absolute path.
        
    -   Any inherit statement consults this cache.
        
    -   This breaks your current recursive compile loops.
        
2.  **Phase 2: Compile-Time Function Binding**
    
    -   Instead of `find_parent_function()` at runtime, resolve and bind parent calls in the compiler.
        
    -   Emit explicit bytecode instructions (`CALL_PARENT_FUNC`, `CALL_NAMED_PARENT`) that hold _pointers_ or _indexes_ into the proto’s function table.
        
    -   Runtime only executes those—no lookups.
        

----------

### 🧩 Suggested Architectural Refactor

#### 1. Global Proto Cache

Create a `proto_cache` (hash map):  
`pathname → proto*`

`proto_t *get_proto(const  char *path) { proto_t *p = hash_find(proto_cache, path); if (p) return p;
    p = compile_proto(path);
    hash_insert(proto_cache, path, p); return p;
}` 

-   `compile_proto()` runs the full parser once per file.
    
-   Each proto stores: variable table, function table, inherit list, and bytecode.
    
-   Make protos immutable once finalized.
    

----------

#### 2. Simplify Inheritance in Parser

`void  add_inherit(const  char *path) { proto_t *parent = get_proto(path);
    vector_push(current_proto->inherits, parent);
    copy_parent_vars(parent, current_proto);
}` 

-   No recompile loops.
    
-   Each proto knows its parents.
    
-   You can still do recursive dependency resolution safely (acyclic graph).
    

----------

#### 3. Compile-Time Resolution for `::func()` and `parent::func()`

When encountering `::func()`:

`inherit_t *first_parent = current_proto->inherits[0]; funcdef_t *f = lookup_func(first_parent, "func");
emit_call_parent(f->index);` 

When encountering `parent::func()`:

`inherit_t *parent = find_inherit_by_name(current_proto, "parent"); funcdef_t *f = lookup_func(parent->proto, "func");
emit_call_named_parent(f->index);` 

No runtime search.  
You emit a _direct call_ instruction with a known function index.

----------

#### 4. Bytecode and Runtime Changes

Add opcodes:

`CALL_PARENT_FUNC <func_index>
CALL_NAMED_PARENT <func_index>` 

At runtime:

`case CALL_PARENT_FUNC:
    func = current_proto->inherits[0]->func_table[func_index];
    call_function(func, obj); break;` 

There’s **no lookup** or traversal at runtime anymore.

----------

#### 5. Tokenizer Isolation

When compiling inherits recursively, push/pop tokenizer state:

`parse_state_t saved = current_parse_state;
compile_proto(path);
restore_parse_state(saved);` 

This stops recursive `inherit` parsing from corrupting the main parser’s position.

----------

### 🧠 Conceptual Payoff

Old Behavior

New Behavior

Dynamic lookup at runtime

Direct call at compile time

Recursive descent recompile

Single compile + cached proto

Smart search for parent

Compiler-encoded linkage

Reentrant tokenizer corruption

Isolated parser state

Memory blowup

Shared immutable protos

----------

### ⚙️ Incremental Refactor Steps

1.  Implement the **global proto cache** (fast, least invasive).
    
2.  Remove recursive proto compilation.
    
3.  Add compiler pass for resolving `::` and `parent::` calls.
    
4.  Simplify runtime dispatch to pointer calls.
    
5.  Clean up old runtime lookup logic and current_func tracking.
    

----------

If you’d like, I can sketch a minimal **pseudo-code outline** of what your new parser/compiler loop should look like after these changes — showing the order of proto caching, parsing, and bytecode emission.  
Would you like that?
