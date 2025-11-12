# Implementation Spec: help_d.c - Dynamic Help System

**Priority:** High (Phase 2)  
**Status:** Not Started  
**Estimated Time:** 3-4 hours  
**Dependencies:** 
- cmd_d.c ✅ (has get_search_path() and command discovery)
- get_dir() efun ✅ (completed Nov 10, 2025)

---

## Overview

The help daemon (`/sys/daemons/help_d.c`) provides a dynamic, self-documenting help system that automatically discovers available commands and queries them for help text. Features include fuzzy matching for "did you mean?" suggestions, category organization, and help file indexing.

## Core Requirements

### 1. Dynamic Command Help
- Scan available commands from cmd_d search paths
- Call `query_help()` on each command object
- Cache help text for performance
- Auto-refresh on `help reload`

### 2. Fuzzy Matching "Did You Mean?"
- Levenshtein distance algorithm for similarity
- Suggest closest matches when topic not found
- Configurable similarity threshold
- Handle typos and partial matches

### 3. Help File Indexing
- Index static help files in `/doc/help/`
- Support categories and subcategories
- Keyword tagging
- Full-text search

### 4. Category Organization
- Group commands by role (player, wizard, admin)
- Group by topic (communication, movement, inventory, etc.)
- Dynamic category discovery from commands
- `help topics` to list all categories

## API Functions

```c
// Main help functions
string get_help(string topic, object player);
string *suggest_topics(string partial);
void display_help(string topic, object player);

// Administration
void rehash_help();
void reload_help();
mapping query_help_index();

// Categories
string *query_categories();
string *query_topics_in_category(string category);

// Statistics
int query_help_entries();
mapping query_popular_topics();
```

## Implementation Details

### Data Structures

```c
// Help index (topic → help text)
mapping help_index;  // ([ "who" : "Shows connected players..." ])

// Command objects cache
mapping command_cache;  // ([ "who" : "/cmds/player/_who" ])

// Category mapping
mapping categories;  // ([ "communication" : ({ "say", "tell", "gossip" }) ])

// Fuzzy match cache (for performance)
mapping fuzzy_cache;  // ([ "woh" : ({ "who", "quit" }) ])

// Usage statistics
mapping topic_views;  // ([ "who" : 42 ])
```

### Help Reload System

```c
void reload_help() {
    // Clear all caches
    help_index = ([]);
    command_cache = ([]);
    categories = ([]);
    fuzzy_cache = ([]);
    
    // Rebuild from commands
    index_commands();
    
    // Index static help files
    index_help_files();
    
    // Build category mappings
    build_categories();
    
    logger(LOG_INFO, "Help system reloaded");
}

void index_commands() {
    object player;
    string *paths, *files;
    int i, j;
    
    // Get all command search paths from cmd_d
    paths = ({
        "/cmds/player",
        "/cmds/wizard",
        "/cmds/admin"
    });
    
    for (i = 0; i < sizeof(paths); i++) {
        files = get_dir(paths[i] + "/*.c");
        
        if (!files) continue;
        
        for (j = 0; j < sizeof(files); j++) {
            string cmd_name, cmd_path, help_text;
            object cmd_obj;
            
            // Skip non-command files
            if (leftstr(files[j], 1) != "_") continue;
            
            // Get command name (strip _ prefix and .c suffix)
            cmd_name = midstr(files[j], 1, strlen(files[j]) - 3);
            cmd_path = paths[i] + "/" + files[j];
            
            // Try to load command object
            cmd_obj = find_object(cmd_path);
            if (!cmd_obj) {
                cmd_obj = load_object(cmd_path);
            }
            
            if (!cmd_obj) continue;
            
            // Query help text from command
            help_text = cmd_obj->query_help();
            if (help_text && help_text != "") {
                help_index[cmd_name] = help_text;
                command_cache[cmd_name] = cmd_path;
            }
        }
    }
}
```

### Fuzzy Matching Algorithm

```c
// Levenshtein distance - measures edit distance between strings
int levenshtein_distance(string s1, string s2) {
    int len1, len2, i, j;
    int *prev_row, *curr_row, *temp;
    int cost;
    
    len1 = strlen(s1);
    len2 = strlen(s2);
    
    // Create arrays for dynamic programming
    prev_row = allocate(len2 + 1);
    curr_row = allocate(len2 + 1);
    
    // Initialize first row
    for (j = 0; j <= len2; j++) {
        prev_row[j] = j;
    }
    
    // Calculate distances
    for (i = 1; i <= len1; i++) {
        curr_row[0] = i;
        
        for (j = 1; j <= len2; j++) {
            // Cost is 0 if characters match, 1 if they don't
            cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            
            // Minimum of: deletion, insertion, substitution
            curr_row[j] = min(
                prev_row[j] + 1,      // deletion
                curr_row[j-1] + 1,    // insertion
                prev_row[j-1] + cost  // substitution
            );
        }
        
        // Swap rows
        temp = prev_row;
        prev_row = curr_row;
        curr_row = temp;
    }
    
    return prev_row[len2];
}

// Find similar topics using fuzzy matching
string *suggest_topics(string partial) {
    string *all_topics, *suggestions;
    int i, distance, threshold;
    mapping scored;
    
    // Check cache first
    if (fuzzy_cache[partial]) {
        return fuzzy_cache[partial];
    }
    
    all_topics = keys(help_index);
    suggestions = ({});
    scored = ([]);
    
    // Threshold: allow up to 30% character differences
    threshold = strlen(partial) / 3;
    if (threshold < 1) threshold = 1;
    if (threshold > 3) threshold = 3;
    
    // Score all topics
    for (i = 0; i < sizeof(all_topics); i++) {
        distance = levenshtein_distance(
            lowercase(partial),
            lowercase(all_topics[i])
        );
        
        // Only include if within threshold
        if (distance <= threshold) {
            scored[all_topics[i]] = distance;
        }
    }
    
    // Sort by distance (closest first)
    suggestions = sort_array(keys(scored), 
        (: scored[$1] < scored[$2] :));
    
    // Limit to top 5 suggestions
    if (sizeof(suggestions) > 5) {
        suggestions = suggestions[0..4];
    }
    
    // Cache result
    fuzzy_cache[partial] = suggestions;
    
    return suggestions;
}
```

### Main Help Lookup

```c
string get_help(string topic, object player) {
    string help_text, *suggestions;
    int role;
    
    if (!topic || topic == "") {
        return get_main_help(player);
    }
    
    // Normalize topic
    topic = lowercase(trim(topic));
    
    // Direct lookup
    help_text = help_index[topic];
    
    if (help_text) {
        // Track usage
        if (!topic_views[topic]) topic_views[topic] = 0;
        topic_views[topic]++;
        
        return help_text;
    }
    
    // Not found - try fuzzy matching
    suggestions = suggest_topics(topic);
    
    if (sizeof(suggestions) == 0) {
        return sprintf("No help available for '%s'.\n\n" +
                      "Type 'help topics' to see all available topics.", 
                      topic);
    }
    
    // Found suggestions
    if (sizeof(suggestions) == 1) {
        return sprintf("Topic '%s' not found. Did you mean '%s'?\n" +
                      "Type 'help %s' to see that topic.",
                      topic, suggestions[0], suggestions[0]);
    }
    
    // Multiple suggestions
    return sprintf("Topic '%s' not found. Did you mean:\n%s\n\n" +
                  "Type 'help <topic>' to see help for that topic.",
                  topic, 
                  "  " + implode(suggestions, "\n  "));
}

void display_help(string topic, object player) {
    string help_text;
    
    help_text = get_help(topic, player);
    
    if (!help_text) {
        tell(player, "No help available.");
        return;
    }
    
    // Use pager for long help text
    if (strlen(help_text) > 1000 && player->query_pager_enabled()) {
        player->start_pager(help_text);
    } else {
        tell(player, help_text);
    }
}
```

### Category System

```c
void build_categories() {
    string *topics, category;
    object cmd_obj;
    int i;
    
    categories = ([]);
    topics = keys(help_index);
    
    for (i = 0; i < sizeof(topics); i++) {
        // Get command object
        if (!command_cache[topics[i]]) continue;
        
        cmd_obj = find_object(command_cache[topics[i]]);
        if (!cmd_obj) continue;
        
        // Query category from command
        category = cmd_obj->query_category();
        if (!category) {
            // Default category based on path
            if (strstr(command_cache[topics[i]], "/cmds/admin/") != -1) {
                category = "administration";
            } else if (strstr(command_cache[topics[i]], "/cmds/wizard/") != -1) {
                category = "wizard";
            } else {
                category = "general";
            }
        }
        
        // Add to category
        if (!categories[category]) {
            categories[category] = ({});
        }
        categories[category] += ({ topics[i] });
    }
}

string *query_categories() {
    return sort_array(keys(categories));
}

string *query_topics_in_category(string category) {
    if (!categories[category]) return ({});
    return sort_array(categories[category]);
}
```

### Static Help Files

```c
void index_help_files() {
    string *files, *lines, topic, content;
    int i, j;
    
    // Look for help files in /doc/help/
    files = get_dir("/doc/help/*.txt");
    
    if (!files) return;
    
    for (i = 0; i < sizeof(files); i++) {
        // Topic is filename without extension
        topic = leftstr(files[i], strlen(files[i]) - 4);
        
        // Read file content
        content = read_file("/doc/help/" + files[i]);
        
        if (content && content != "") {
            // Don't override command help
            if (!help_index[topic]) {
                help_index[topic] = content;
            }
        }
    }
}
```

### Main Help Text

```c
string get_main_help(object player) {
    string *categories_list, text;
    int i, role;
    
    role = player->query_role();
    categories_list = query_categories();
    
    text = "=== HELP SYSTEM ===\n\n";
    text += "Usage: help <topic>\n";
    text += "       help topics\n";
    text += "       help <category>\n\n";
    
    text += "Available Categories:\n";
    for (i = 0; i < sizeof(categories_list); i++) {
        int count = sizeof(query_topics_in_category(categories_list[i]));
        text += sprintf("  %-20s (%d topics)\n", 
                       capitalize(categories_list[i]), count);
    }
    
    text += "\nPopular Topics:\n";
    text += "  who, say, go, look, inventory, quit\n\n";
    
    text += "Tip: The help system supports fuzzy matching!\n";
    text += "     Try 'help woh' and see what happens.\n";
    
    if (role >= ROLE_WIZARD) {
        text += "\nWizard: Use 'help reload' to refresh help index.\n";
    }
    
    return text;
}
```

## Command Interface (_help.c)

```c
// /cmds/player/_help.c
int main(string args) {
    object player = this_player();
    
    if (!args || args == "") {
        HELP_D->display_help("", player);
        return 1;
    }
    
    // Special commands
    if (args == "reload") {
        if (player->query_role() < ROLE_WIZARD) {
            tell(player, "Permission denied.");
            return 1;
        }
        
        HELP_D->reload_help();
        tell(player, "Help system reloaded.");
        return 1;
    }
    
    if (args == "topics") {
        display_topics(player);
        return 1;
    }
    
    // Regular help lookup
    HELP_D->display_help(args, player);
    return 1;
}

void display_topics(object player) {
    string *categories, *topics, text;
    int i, j;
    
    categories = HELP_D->query_categories();
    
    text = "=== AVAILABLE HELP TOPICS ===\n\n";
    
    for (i = 0; i < sizeof(categories); i++) {
        topics = HELP_D->query_topics_in_category(categories[i]);
        
        text += sprintf("\n%s:\n", capitalize(categories[i]));
        text += "  " + implode(topics, ", ") + "\n";
    }
    
    tell(player, text);
}

string query_help() {
    return 
"HELP - Display help information\n\n" +
"Usage:\n" +
"  help              Show main help menu\n" +
"  help <topic>      Show help for specific topic\n" +
"  help topics       List all available topics\n" +
"  help reload       Reload help system (wizard+)\n\n" +
"The help system automatically discovers commands and supports\n" +
"fuzzy matching to suggest similar topics when exact matches\n" +
"aren't found.\n\n" +
"Examples:\n" +
"  help who          Show help for 'who' command\n" +
"  help woh          Fuzzy match suggests 'who'\n" +
"  help topics       List all help topics\n";
}

string query_category() {
    return "information";
}
```

## Command query_help() Pattern

All commands should implement:

```c
string query_help() {
    return
"<COMMAND NAME> - <brief description>\n\n" +
"Usage:\n" +
"  <command> [args]\n\n" +
"<Detailed description>\n\n" +
"Examples:\n" +
"  <example 1>\n" +
"  <example 2>\n";
}

string query_category() {
    return "<category>";  // communication, movement, inventory, etc.
}
```

## Testing Plan

### Unit Tests
1. Levenshtein distance calculation
2. Fuzzy matching with various inputs
3. Category organization
4. Cache invalidation

### Integration Tests
1. Help reload discovers all commands
2. query_help() called on all commands
3. Fuzzy matching suggests correct topics
4. Static help files loaded

### Edge Cases
1. Command with no query_help()
2. Empty search string
3. Very long help text (pager)
4. Special characters in topic names

## Performance Considerations

- **Caching**: Help text cached after first load
- **Lazy loading**: Commands only loaded when help requested
- **Fuzzy cache**: Expensive Levenshtein calculations cached
- **O(n log n) sorting**: Only done once per reload
- **Efficient string operations**: Use NetCI's leftstr, rightstr, midstr

## File Locations

- **Daemon**: `/sys/daemons/help_d.c`
- **Command**: `/cmds/player/_help.c`
- **Static Help**: `/doc/help/*.txt`

## Configuration

```c
// In /include/config.h
#define HELP_D "/sys/daemons/help_d"
#define HELP_DIR "/doc/help"
```

## Future Enhancements

1. **Full-text search** - Search help content, not just topics
2. **See also links** - Related topics
3. **Apropos command** - Search by keyword
4. **Help history** - Track what player has read
5. **Interactive help** - Navigate with menu system
6. **Help contributions** - Players can submit help text
7. **Multi-language** - Support translations

## Notes

- Levenshtein algorithm provides better fuzzy matching than simple substring
- query_help() pattern allows commands to self-document
- Dynamic discovery means new commands automatically get help
- Categories make browsing large help systems manageable
- Fuzzy matching is user-friendly for typos
