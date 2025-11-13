/* user.c - User connection object for Melville/NetCI
 *
 * This object handles the network connection and login process.
 * It is NOT the player's in-world body - that's player.c.
 *
 * Responsibilities:
 * - Handle TCP connection (via connect() callback)
 * - Manage login/authentication flow
 * - Transfer connection to player object after login
 * - Handle input redirection during login
 *
 * Flow:
 * 1. boot.c::connect() creates new("/sys/user")
 * 2. boot.c calls reconnect_device(user) and user.connect()
 * 3. user.c handles login via redirect_input()
 * 4. After auth, reconnect_device(player) and player.connect()
 * 5. user.c destructs itself
 */

#include <std.h>
#include <config.h>
#include <roles.h>

/* Connection state */
string player_name;
string temp_password;
int login_attempts;

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

/* Called when connection is established by boot.c
 * This is AFTER reconnect_device() has transferred the connection to us
 */
connect() {
    mapping term_info;
    string term_type;
    string color_support;
    
    /* Display welcome message */
    send_device("\n");
    send_device("========================================\n");
    send_device("  Welcome to Melville MUD\n");
    send_device("========================================\n");
    send_device("\n");
    
    /* Display terminal detection info */
    term_info = query_terminal(this_object());
    if (term_info) {
        term_type = term_info["term_type"];
        if (!term_type || term_type == "") {
            term_type = "unknown";
        }
        
        /* Determine color support from terminal type */
        if (instr(term_type, 0, "256color") != -1) {
            color_support = "ANSI 256 colors";
        } else if (instr(term_type, 0, "color") != -1 || 
                   instr(term_type, 0, "ansi") != -1 ||
                   instr(term_type, 0, "xterm") != -1 ||
                   instr(term_type, 0, "mud") != -1) {
            color_support = "ANSI 16 colors";
        } else {
            color_support = "none detected";
        }
        
        send_device("[ Login ] Terminal type: " + term_type + "\n");
        send_device("[ Login ] Terminal color support: " + color_support + "\n");
        send_device("\n");
    }
    
    /* Start login process */
    send_device("Enter your character name: ");
    redirect_input("get_name");
}

/* ========================================================================
 * LOGIN FLOW - NAME ENTRY
 * ======================================================================== */

get_name(input) {
    string name_lower;
    
    if (!input || strlen(input) < 3) {
        send_device("Name must be at least 3 characters.\n");
        send_device("Enter your character name: ");
        redirect_input("get_name");
        return;
    }
    
    if (strlen(input) > 15) {
        send_device("Name must be 15 characters or less.\n");
        send_device("Enter your character name: ");
        redirect_input("get_name");
        return;
    }
    
    /* Validate name characters */
    if (!is_legal(input)) {
        send_device("Name contains invalid characters.\n");
        send_device("Enter your character name: ");
        redirect_input("get_name");
        return;
    }
    
    /* Normalize name */
    name_lower = downcase(input);
    player_name = name_lower;
    
    /* Check if player exists */
    if (player_exists(player_name)) {
        /* Existing player - ask for password */
        send_device("Enter your password: ");
        login_attempts = 0;
        redirect_input("check_password");
    } else {
        /* New player - create account */
        send_device("New character! Choose a password: ");
        redirect_input("get_new_password");
    }
}

/* ========================================================================
 * LOGIN FLOW - EXISTING PLAYER
 * ======================================================================== */

check_password(input) {
    object player;
    
    if (!input) {
        send_device("Password cannot be empty.\n");
        send_device("Enter your password: ");
        redirect_input("check_password");
        return;
    }
    
    /* Load player object */
    player = load_player(player_name);
    if (!player) {
        send_device("Error loading player data.\n");
        syslog("Failed to load player: " + player_name);
        disconnect_device();
        destruct(this_object());
        return;
    }
    
    /* Verify password */
    if (!player.check_password(input)) {
        login_attempts++;
        
        if (login_attempts >= 3) {
            send_device("Too many failed attempts.\n");
            syslog("Failed login attempts for: " + player_name);
            disconnect_device();
            destruct(player);
            destruct(this_object());
            return;
        }
        
        send_device("Incorrect password.\n");
        send_device("Enter your password: ");
        redirect_input("check_password");
        return;
    }
    
    /* Successful login! */
    syslog(capitalize(player_name) + " logged in");
    transfer_to_player(player);
}

/* ========================================================================
 * LOGIN FLOW - NEW PLAYER
 * ======================================================================== */

get_new_password(input) {
    if (!input || strlen(input) < 6) {
        send_device("Password must be at least 6 characters.\n");
        send_device("Choose a password: ");
        redirect_input("get_new_password");
        return;
    }
    
    /* Store temporarily for confirmation */
    temp_password = input;
    
    send_device("Confirm password: ");
    redirect_input("confirm_password");
}

confirm_password(input) {
    object player;
    
    if (input != temp_password) {
        send_device("Passwords don't match.\n");
        send_device("Choose a password: ");
        temp_password = NULL;
        redirect_input("get_new_password");
        return;
    }
    
    /* Create new player */
    player = create_player(player_name, temp_password);
    temp_password = NULL;  /* Clear from memory */
    
    if (!player) {
        send_device("Error creating character.\n");
        syslog("Failed to create player: " + player_name);
        disconnect_device();
        destruct(this_object());
        return;
    }
    
    send_device("\nWelcome to Melville, " + capitalize(player_name) + "!\n\n");
    syslog("New player created: " + player_name);
    
    transfer_to_player(player);
}

/* ========================================================================
 * PLAYER MANAGEMENT
 * ======================================================================== */

/* Check if player save file exists */
player_exists(name) {
    string path;
    int flags;
    
    path = PLAYER_SAVE_DIR + name + ".o";
    
    /* Use fstat to check if file exists (-1 means doesn't exist) */
    flags = fstat(path);
    if (flags != -1) {
        return 1;
    }
    
    return 0;
}

/* Load existing player object */
load_player(name) {
    object player;
    string path;
    
    path = PLAYER_PATH;
    
    /* Clone player object */
    player = new(path);
    if (!player) {
        return NULL;
    }
    
    /* Load saved data */
    if (!player.load_data(name)) {
        destruct(player);
        return NULL;
    }
    
    return player;
}

/* Create new player object */
create_player(name, password) {
    object player;
    string path;
    
    path = PLAYER_PATH;
    
    /* Clone player object */
    player = new(path);
    if (!player) {
        return NULL;
    }
    
    /* Initialize new player */
    player.set_name(name);
    player.set_password(password);
    player.set_role(ROLE_PLAYER);
    
    /* Save initial data */
    if (!player.save_data()) {
        destruct(player);
        return NULL;
    }
    
    return player;
}

/* Transfer connection from user to player */
transfer_to_player(player) {
    object users_d;
    
    if (!player) {
        send_device("Error: Invalid player object.\n");
        disconnect_device();
        destruct(this_object());
        return;
    }
    
    /* Transfer the device connection to player */
    if (reconnect_device(player)) {
        send_device("Error transferring connection.\n");
        syslog("reconnect_device failed for: " + player_name);
        disconnect_device();
        destruct(player);
        destruct(this_object());
        return;
    }
    
    /* Let player handle connection now */
    player.connect();
    
    /* Notify users daemon of new login */
    users_d = atoo(USERS_D);
    if (users_d) {
        call_other(users_d, "user_logged_in", player);
    }
    
    /* We're done - destruct user object */
    destruct(this_object());
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* NOTE: capitalize() is available from auto.c (automatically attached) */

/* ========================================================================
 * DISCONNECTION
 * ======================================================================== */

/* Called when connection is lost */
disconnect() {
    /* Clean up */
    player_name = NULL;
    temp_password = NULL;
    
    destruct(this_object());
}
