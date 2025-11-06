/* container.c - Container object for Melville/NetCI
 *
 * This inheritable provides inventory management for objects that can
 * contain other objects. This includes:
 * - Rooms (contain players and objects)
 * - Players (carry items)
 * - Bags, chests, etc. (hold items)
 *
 * NOTE: This does NOT provide opening/closing/locking functionality.
 * Create a separate bag.c inheritable if you need that.
 *
 * IMPLEMENTATION NOTES:
 * ---------------------
 * This uses a HYBRID approach for inventory management:
 * 
 * 1. We maintain our own object *inventory array for fast access/iteration
 * 2. We use move_object() underneath so driver's contents list stays in sync
 * 3. receive_object() and release_object() are OUR hooks (not driver hooks)
 * 4. The move() function in object.c calls these hooks, then move_object()
 *
 * This means:
 * - query_inventory() returns our fast array
 * - contents(obj) + next_object() also works (driver's linked list)
 * - Both should stay in sync as long as objects use move()
 * - If they get out of sync, call sync_inventory() to rebuild from driver
 *
 * Why this approach?
 * - Fast array-based iteration (no linked list walking)
 * - Driver's contents list stays accurate for low-level code
 * - Our hooks provide policy (can reject moves)
 * - Best of both worlds!
 */

#include <std.h>

/* Inventory stored as dynamic array for fast access */
object *inventory;

/* ========================================================================
 * INVENTORY MANAGEMENT
 * ======================================================================== */

/* Receive an object into inventory
 * Called when an object tries to move into this container
 * Returns: 1 to accept, 0 to reject
 * 
 * NOTE: This is called BEFORE move_object() to check if we accept the object.
 * The actual move_object() call happens in the move() function.
 */
receive_object(ob) {
    if (!ob) return 0;
    
    /* Initialize inventory if needed */
    if (!inventory) inventory = ({});
    
    /* Check if already in inventory */
    if (member_array(ob, inventory) != -1) {
        return 1;  /* Already here */
    }
    
    /* Add to our tracking array */
    inventory = inventory + ({ ob });
    
    return 1;
}

/* Release an object from inventory
 * Called when an object tries to leave this container
 * Returns: 1 to allow, 0 to prevent
 */
release_object(ob) {
    if (!ob) return 0;
    if (!inventory) return 0;
    
    /* Check if object is in inventory */
    if (member_array(ob, inventory) == -1) {
        return 0;  /* Not here */
    }
    
    /* Remove from inventory */
    inventory = inventory - ({ ob });
    
    return 1;
}

/* Sync our inventory array with driver's contents list
 * Call this if you suspect they're out of sync
 */
sync_inventory() {
    object curr;
    
    /* Rebuild from driver's contents list */
    inventory = ({});
    
    curr = contents(this_object());
    while (curr) {
        inventory = inventory + ({ curr });
        curr = next_object(curr);
    }
}

/* Query inventory - returns array of objects */
query_inventory() {
    if (!inventory) return ({});
    return inventory;
}

get_inventory() {
    return query_inventory();
}

/* Get inventory count */
query_inventory_count() {
    if (!inventory) return 0;
    return sizeof(inventory);
}

/* Query inventory using driver's contents iterator
 * This is slower but guaranteed to be accurate
 */
query_inventory_safe() {
    object curr, *inv;
    
    inv = ({});
    curr = contents(this_object());
    while (curr) {
        inv = inv + ({ curr });
        curr = next_object(curr);
    }
    
    return inv;
}

/* ========================================================================
 * PRESENT FUNCTION
 * ======================================================================== */

/* Find an object in inventory by ID
 * Special cases: "me" returns this_player() if present
 *                "here" returns this container
 */
present(id) {
    int i;
    object ob;
    
    if (!id) return NULL;
    
    /* Special case: "me" */
    if (id == "me") {
        ob = this_player();
        if (ob && member_array(ob, inventory) != -1) {
            return ob;
        }
        return NULL;
    }
    
    /* Special case: "here" */
    if (id == "here") {
        return this_object();
    }
    
    /* Search inventory */
    if (!inventory) return NULL;
    
    for (i = 0; i < sizeof(inventory); i++) {
        ob = inventory[i];
        if (ob) {
            if (call_other(ob, "id", id)) {
                return ob;
            }
        }
    }
    
    return NULL;
}

/* ========================================================================
 * INVENTORY QUERIES
 * ======================================================================== */

/* Check if inventory is empty */
is_empty() {
    return (!inventory || sizeof(inventory) == 0);
}

/* Get first object in inventory */
first_inventory() {
    if (!inventory || sizeof(inventory) == 0) return NULL;
    return inventory[0];
}

/* Get next object in inventory after given object */
next_inventory(ob) {
    int pos;
    
    if (!ob) return first_inventory();
    if (!inventory) return NULL;
    
    pos = member_array(ob, inventory);
    if (pos == -1 || pos >= sizeof(inventory) - 1) {
        return NULL;
    }
    
    return inventory[pos + 1];
}

/* ========================================================================
 * CLEANUP
 * ======================================================================== */

/* Clean up inventory data when container is destructed */
destruct_container() {
    /* Arrays are automatically freed */
    inventory = NULL;
}
