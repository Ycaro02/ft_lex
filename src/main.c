#include "../include/log.h"
#include "../include/nfa.h"

// typedef struct ClassDef {
//     char charset[1024];
//     s8   reverse_match;
// } ClassDef;

// s8 parse_class_exp(char *exp) {
//     ClassDef class_def = { .charset = "", .reverse_match = 0 };
//     int i = 0, j = 0;

//     if (exp[i] == '^') {
//         class_def.reverse_match = 1;
//         i++;
//     }

//     int exp_len = strlen(exp);

//     while (exp[i] != 0) {

//         if (i + 2 < exp_len) {
//             if (exp[i + 1] == '-' && exp[i + 2] && exp[i] < exp[i + 2]) {
//                 for (char c = exp[i]; c <= exp[i + 2]; c++) {
//                     class_def.charset[j] = c;
//                     j++;
//                 }
//                 i += 3;
//             }
//         } else {
//             class_def.charset[j] = exp[i];
//             j++;
//             i++;

//         }
//     }

//     INFO("CHARSET: %s\n", class_def.charset);
//     return (1);

// }


/**
 * @brief Get pointer to global state ID counter
 * @return Pointer to static state ID counter
 */
int *get_state_id_counter() {
    static int state_id = 0;
    return (&state_id);
}

/* Global NFA - all states are stored here */
static NFA g_nfa = {0};

/**
 * @brief Initialize the global NFA with specified capacity
 * @param capacity Initial capacity for the states array
 */
void nfa_init(int capacity) {
    g_nfa.states = calloc(capacity, sizeof(NFAState));
    g_nfa.state_count = 0;
    g_nfa.capacity = capacity;
    g_nfa.start_id = -1;
}

/**
 * @brief Free all memory allocated for the NFA
 * 
 * Frees the transitions array of each state, then the states array itself.
 */
void nfa_free(void) {
    // Free transitions array for each state
    for (int i = 0; i < g_nfa.state_count; i++) {
        free(g_nfa.states[i].trans);
    }
    free(g_nfa.states);
    g_nfa.states = NULL;
    g_nfa.state_count = 0;
}

/**
 * @brief Create a new NFA state
 * @param is_final Whether this state is a final/accepting state
 * @return ID of the newly created state
 * 
 * Automatically grows the states array if capacity is reached.
 * Initializes the state with a dynamic transitions array.
 */
int create_state(int is_final) {
    if (g_nfa.state_count >= g_nfa.capacity) {
        g_nfa.capacity *= 2;
        g_nfa.states = realloc(g_nfa.states, g_nfa.capacity * sizeof(NFAState));
    }
    
    int id = g_nfa.state_count++;
    g_nfa.states[id].id = id;
    g_nfa.states[id].is_final = is_final;
    g_nfa.states[id].trans = malloc(INITIAL_TRANSITIONS_CAPACITY * sizeof(Transition));
    g_nfa.states[id].trans_count = 0;
    g_nfa.states[id].trans_capacity = INITIAL_TRANSITIONS_CAPACITY;
    
    return id;
}

/**
 * @brief Add a transition from one state to another
 * @param from_id ID of the source state
 * @param c Character to match (0 for epsilon transition)
 * @param to_id ID of the destination state
 * 
 * Automatically grows the transitions array if capacity is reached.
 * This allows unlimited transitions per state.
 */
void add_transition(int from_id, char c, int to_id) {
    NFAState *s = &g_nfa.states[from_id];
    
    // Reallocate if necessary (double the capacity)
    if (s->trans_count >= s->trans_capacity) {
        s->trans_capacity *= 2;
        s->trans = realloc(s->trans, s->trans_capacity * sizeof(Transition));
        if (!s->trans) {
            fprintf(stderr, "ERROR: Failed to realloc transitions for state %d\n", from_id);
            exit(1);
        }
    }
    
    s->trans[s->trans_count].c = c;
    s->trans[s->trans_count].to_id = to_id;
    s->trans_count++;
}

// ============================================================================
// FRAGMENT HANDLING
// ============================================================================

/**
 * @brief Create a new NFA fragment
 * @param start_id ID of the fragment's start state
 * @return New NFAFragment with dynamic output array
 */
NFAFragment frag_create(int start_id) {
    NFAFragment f;
    f.start_id = start_id;
    f.out_ids = malloc(8 * sizeof(int));
    f.out_count = 0;
    f.out_capacity = 8;
    return f;
}

/**
 * @brief Add an output state to a fragment
 * @param f Pointer to the fragment
 * @param state_id ID of the state to add to outputs
 * 
 * Automatically grows the output array if capacity is reached.
 */
void frag_add_out(NFAFragment *f, int state_id) {
    if (f->out_count >= f->out_capacity) {
        f->out_capacity *= 2;
        f->out_ids = realloc(f->out_ids, f->out_capacity * sizeof(int));
    }
    f->out_ids[f->out_count++] = state_id;
}

/**
 * @brief Free memory allocated for a fragment
 * @param f Pointer to the fragment to free
 */
void frag_free(NFAFragment *f) {
    free(f->out_ids);
    f->out_ids = NULL;
    f->out_count = 0;
}

// ============================================================================
// THOMPSON CONSTRUCTION - BASIC BUILDING BLOCKS
// ============================================================================

/**
 * @brief Create NFA fragment for a single character
 * @param c Character to match
 * @return Fragment with start -> c -> end
 */
NFAFragment nfa_char(char c) {
    int s = create_state(0);
    int e = create_state(0);
    add_transition(s, c, e);
    
    NFAFragment frag = frag_create(s);
    frag_add_out(&frag, e);
    
    return frag;
}

/**
 * @brief Concatenate two NFA fragments (AB)
 * @param left Left fragment (A)
 * @param right Right fragment (B)
 * @return Combined fragment representing the concatenation
 * 
 * Connects all outputs of left to the start of right via epsilon transitions.
 */
NFAFragment nfa_concat(NFAFragment left, NFAFragment right) {
    // Connect all outputs of left to the start of right
    for (int i = 0; i < left.out_count; i++) {
        add_transition(left.out_ids[i], 0, right.start_id);
    }
    
    NFAFragment result = frag_create(left.start_id);
    for (int i = 0; i < right.out_count; i++) {
        frag_add_out(&result, right.out_ids[i]);
    }
    
    frag_free(&left);
    frag_free(&right);
    
    return result;
}

/**
 * @brief Create alternation between two NFA fragments (A|B)
 * @param left Left alternative (A)
 * @param right Right alternative (B)
 * @return Fragment with new start branching to both alternatives
 * 
 * Creates a new start state with epsilon transitions to both branches.
 */
NFAFragment nfa_alt(NFAFragment left, NFAFragment right) {
    int start = create_state(0);
    
    // Epsilon transitions to both branches
    add_transition(start, 0, left.start_id);
    add_transition(start, 0, right.start_id);
    
    NFAFragment result = frag_create(start);
    
    // All outputs from both branches become fragment outputs
    for (int i = 0; i < left.out_count; i++) {
        frag_add_out(&result, left.out_ids[i]);
    }
    for (int i = 0; i < right.out_count; i++) {
        frag_add_out(&result, right.out_ids[i]);
    }
    
    frag_free(&left);
    frag_free(&right);
    
    return result;
}

/**
 * @brief Apply Kleene star operator to a fragment (A*)
 * @param frag Fragment to apply star to
 * @return New fragment matching zero or more repetitions
 * 
 * Creates new start and end states. Start can skip to end (zero matches)
 * or enter fragment. Fragment outputs can loop back or exit.
 */
NFAFragment nfa_star(NFAFragment frag) {
    int start = create_state(0);
    int end = create_state(0);
    
    // Epsilon: start -> frag.start | start -> end
    add_transition(start, 0, frag.start_id);
    add_transition(start, 0, end);
    
    // All outputs: -> frag.start (loop) | -> end (exit)
    for (int i = 0; i < frag.out_count; i++) {
        add_transition(frag.out_ids[i], 0, frag.start_id);
        add_transition(frag.out_ids[i], 0, end);
    }
    
    NFAFragment result = frag_create(start);
    frag_add_out(&result, end);
    
    frag_free(&frag);
    return result;
}

/**
 * @brief Apply plus operator to a fragment (A+)
 * @param frag Fragment to apply plus to
 * @return New fragment matching one or more repetitions
 * 
 * Similar to star but requires at least one match (no direct skip to end).
 */
NFAFragment nfa_plus(NFAFragment frag) {
    int end = create_state(0);
    
    // All outputs: -> frag.start (loop) | -> end (exit)
    for (int i = 0; i < frag.out_count; i++) {
        add_transition(frag.out_ids[i], 0, frag.start_id);
        add_transition(frag.out_ids[i], 0, end);
    }
    
    NFAFragment result = frag_create(frag.start_id);
    frag_add_out(&result, end);
    
    frag_free(&frag);
    return result;
}

/**
 * @brief Apply optional operator to a fragment (A?)
 * @param frag Fragment to make optional
 * @return New fragment matching zero or one occurrence
 * 
 * Creates new start that can skip directly to end or enter fragment.
 */
NFAFragment nfa_optional(NFAFragment frag) {
    int start = create_state(0);
    int end = create_state(0);
    
    add_transition(start, 0, frag.start_id);
    add_transition(start, 0, end);
    
    for (int i = 0; i < frag.out_count; i++) {
        add_transition(frag.out_ids[i], 0, end);
    }
    
    NFAFragment result = frag_create(start);
    frag_add_out(&result, end);
    
    frag_free(&frag);
    return result;
}

// ============================================================================
// CONVERT FROM REGEX TREE
// ============================================================================

/**
 * @brief Convert a regex parse tree to NFA using Thompson's construction
 * @param node Root of the regex parse tree
 * @return NFA fragment representing the regex
 * 
 * Recursively builds the NFA bottom-up from the parse tree,
 * applying operators after building the base fragments.
 */
NFAFragment thompson_from_tree(RegexTreeNode *node) {
    if (!node) {
        fprintf(stderr, "ERROR: Null node\n");
        return frag_create(-1);
    }
    
    NFAFragment frag;
    
    switch (node->type) {
        case REG_CHAR:
            frag = nfa_char(node->c);
            break;
            
        case REG_CONCAT:
            frag = nfa_concat(
                thompson_from_tree(node->left),
                thompson_from_tree(node->right)
            );
            break;
            
        case REG_ALT:
            frag = nfa_alt(
                thompson_from_tree(node->left),
                thompson_from_tree(node->right)
            );
            break;
            
        default:
            fprintf(stderr, "ERROR: Unknown node type %d\n", node->type);
            return frag_create(-1);
    }
    
    // Apply postfix operators
    switch (node->op) {
        case OP_STAR:     frag = nfa_star(frag);     break;
        case OP_PLUS:     frag = nfa_plus(frag);     break;
        case OP_OPTIONAL: frag = nfa_optional(frag); break;
        case OP_NONE:     break;
    }
    
    return frag;
}

/**
 * @brief Finalize the NFA by marking final states
 * @param frag The final fragment to finalize
 * 
 * Sets the global NFA start state and marks all fragment
 * output states as accepting/final states.
 */
void nfa_finalize(NFAFragment *frag) {
    g_nfa.start_id = frag->start_id;
    
    // Mark all output states as final
    for (int i = 0; i < frag->out_count; i++) {
        g_nfa.states[frag->out_ids[i]].is_final = 1;
    }
    
    frag_free(frag);
}

// ============================================================================
// MATCHING (using bitmap for state sets)
// ============================================================================

/**
 * @brief Bitmap for efficient state set representation
 * 
 * Uses 4 64-bit integers to support up to 256 states efficiently.
 */
typedef struct {
    uint64_t bits[4];  // Supports up to 256 states
} StateBitmap;

/**
 * @brief Clear all bits in the bitmap
 * @param b Bitmap to clear
 */
void bitmap_clear(StateBitmap *b) {
    memset(b->bits, 0, sizeof(b->bits));
}

/**
 * @brief Set a bit in the bitmap (add state to set)
 * @param b Bitmap to modify
 * @param id State ID to add
 */
void bitmap_set(StateBitmap *b, int id) {
    if (id >= 0 && id < 256) {
        b->bits[id / 64] |= (1ULL << (id % 64));
    }
}

/**
 * @brief Check if a bit is set in the bitmap (state in set)
 * @param b Bitmap to check
 * @param id State ID to check
 * @return 1 if state is in set, 0 otherwise
 */
int bitmap_is_set(StateBitmap *b, int id) {
    if (id >= 0 && id < 256) {
        return (b->bits[id / 64] & (1ULL << (id % 64))) != 0;
    }
    return 0;
}

/**
 * @brief Compute epsilon closure of a state set
 * @param states Bitmap of states to compute closure for
 * 
 * Iteratively adds all states reachable via epsilon transitions
 * until no new states can be added (fixed point).
 */
void epsilon_closure(StateBitmap *states) {
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < g_nfa.state_count; i++) {
            if (!bitmap_is_set(states, i)) continue;
            
            NFAState *s = &g_nfa.states[i];
            for (int j = 0; j < s->trans_count; j++) {
                if (s->trans[j].c == 0) {  // epsilon transition
                    if (!bitmap_is_set(states, s->trans[j].to_id)) {
                        bitmap_set(states, s->trans[j].to_id);
                        changed = 1;
                    }
                }
            }
        }
    }
}

/**
 * @brief Match input string against the NFA
 * @param input Input string to match
 * @return Pointer to the position after the longest match, or NULL if no match
 * 
 * Uses subset construction to simulate NFA on the input string.
 * Tracks the longest accepting prefix found.
 */
char *match_nfa(char *input) {
    StateBitmap current, next;
    bitmap_clear(&current);
    bitmap_set(&current, g_nfa.start_id);
    epsilon_closure(&current);
    
    char *ptr = input;
    char *last_accept = NULL;
    
    // Check if initial state set contains a final state (empty match)
    for (int i = 0; i < g_nfa.state_count; i++) {
        if (bitmap_is_set(&current, i) && g_nfa.states[i].is_final) {
            last_accept = ptr;
            break;
        }
    }
    
    while (*ptr) {
        bitmap_clear(&next);
        
        // Transitions on current character
        for (int i = 0; i < g_nfa.state_count; i++) {
            if (!bitmap_is_set(&current, i)) continue;
            
            NFAState *s = &g_nfa.states[i];
            for (int j = 0; j < s->trans_count; j++) {
                if (s->trans[j].c == '.' || s->trans[j].c == *ptr) {
                    bitmap_set(&next, s->trans[j].to_id);
                }
            }
        }
        
        epsilon_closure(&next);
        
        // If no states reachable, stop
        int has_states = 0;
        for (int i = 0; i < 4; i++) {
            if (next.bits[i]) {
                has_states = 1;
                break;
            }
        }
        if (!has_states) break;
        
        // Check if any state is final (accepting)
        for (int i = 0; i < g_nfa.state_count; i++) {
            if (bitmap_is_set(&next, i) && g_nfa.states[i].is_final) {
                last_accept = ptr + 1;
                break;
            }
        }
        
        current = next;
        ptr++;
    }
    
    // Check if final state at end of input
    if (*ptr == '\0') {
        for (int i = 0; i < g_nfa.state_count; i++) {
            if (bitmap_is_set(&current, i) && g_nfa.states[i].is_final) {
                last_accept = ptr;
                break;
            }
        }
    }
    
    return last_accept;
}

// ============================================================================
// MATCHING MULTIPLE (anywhere in string)
// ============================================================================

/**
 * @brief Find all matches of the NFA pattern anywhere in the input string
 * @param input Input string to search for matches
 * 
 * Repeatedly attempts to match starting from each position in the input,
 * printing all matches found. Skips zero-length matches to avoid infinite loops.
 */
void match_nfa_anywhere(char *input) {
    char *p = input;
    
    while (*p) {
        char *match = match_nfa(p);
        if (match) {
            int len = match - p;
            if (len == 0) { p++; continue;}  // Skip zero-length matches
            printf("✅ Match Rule: ");
            for (int i = 0; i < len; i++) {
                putchar(p[i]);
            }
            printf("\n");
            p = match;
        } else {
            p++;
        }
    }
}

// ============================================================================
// DEBUG - TREE STYLE PRINTING
// ============================================================================

/**
 * @brief Recursively print NFA states in a tree structure
 * @param state_id ID of the current state to print
 * @param prefix String prefix for indentation
 * @param is_last Whether this is the last child of its parent
 * @param visited Array tracking visited states to avoid cycles
 * @param visited_count Number of visited states
 * 
 * Uses tree-drawing characters (├── └──) for visual hierarchy.
 * Prevents infinite recursion by tracking visited states.
 */
void print_nfa_tree_recursive(int state_id, char* prefix, int is_last, int *visited, int* visited_count) {
    // Check if already visited (avoid cycles)
    for (int i = 0; i < *visited_count; i++) {
        if (visited[i] == state_id) return;
    }
    visited[(*visited_count)++] = state_id;
    
    NFAState *s = &g_nfa.states[state_id];
    
    // Print current state
    printf("%s", prefix);
    if (is_last) {
        printf("└── ");
        strcat(prefix, "    ");
    } else {
        printf("├── ");
        strcat(prefix, "│   ");
    }
    printf("State s%d%s\n", s->id, s->is_final ? " [FINAL]" : "");
    
    // Print transitions
    int trans_count = s->trans_count;
    for (int idx = 0; idx < trans_count; idx++) {
        Transition *t = &s->trans[idx];
        
        char buf[64];
        if (t->c == 0) {
            snprintf(buf, sizeof(buf), "ε");
        } else {
            snprintf(buf, sizeof(buf), "'%c'", t->c);
        }
        
        printf("%s", prefix);
        if (idx == trans_count - 1) {
            printf("└── ");
        } else {
            printf("├── ");
        }
        printf("--%s--> s%d\n", buf, t->to_id);
        
        // Recursively print target state
        char *new_prefix = calloc(strlen(prefix) + 32, sizeof(char));
        strcpy(new_prefix, prefix);
        print_nfa_tree_recursive(t->to_id, new_prefix, idx == trans_count - 1, visited, visited_count);
        free(new_prefix);
    }
}

/**
 * @brief Print the entire NFA structure as a tree
 * 
 * Starts from the NFA start state and recursively prints
 * all reachable states in a tree format.
 */
void print_nfa_tree(void) {
    int visited[2048] = {0};
    char prefix[2048] = {0};
    int visited_count = 0;
    
    printf("NFA Tree:\n");
    print_nfa_tree_recursive(g_nfa.start_id, prefix, 1, visited, &visited_count);
    printf("\n");
}

// Simple flat listing
void print_nfa(void) {
    printf("=== NFA with %d states ===\n", g_nfa.state_count);
    printf("Start: s%d\n", g_nfa.start_id);
    
    for (int i = 0; i < g_nfa.state_count; i++) {
        NFAState *s = &g_nfa.states[i];
        printf("s%d%s:", s->id, s->is_final ? " [FINAL]" : "");
        
        for (int j = 0; j < s->trans_count; j++) {
            char c = s->trans[j].c;
            if (c == 0) {
                printf(" --ε--> s%d", s->trans[j].to_id);
            } else {
                printf(" --'%c'--> s%d", c, s->trans[j].to_id);
            }
        }
        printf("\n");
    }
    printf("\n");
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    set_log_level(L_INFO);
    
    if (argc < 3) {
        INFO("Usage: %s <regex> <str_to_parse>\n", argv[0]);
        return 1;
    }
    
    char *input = argv[2];
    String s = {
        .str = argv[1],
        .pos = 0,
        .len = strlen(argv[1])
    };
    
    INFO("Parsing regex: '%s'\n", s.str);
    INFO("=====================================\n");
    
    // 1. Parser le regex en arbre
    RegexTreeNode *tree = parse_regex(&s);
    
    if (!tree) {
        ERR("Failed to parse regex!\n");
        return 1;
    }
    
    print_regex_tree(tree);
    INFO("Parsing completed successfully!\n");
    INFO("Final position: %d/%d\n", s.pos, (int)strlen(s.str));
    INFO("=====================================\n");
    
    // 2. Initialiser le NFA global
    nfa_init(DEFAULT_NFA_CAPACITY);
    
    // 3. Construire le NFA depuis l'arbre
    NFAFragment frag = thompson_from_tree(tree);
    
    // 4. Finaliser le NFA (marquer les états finaux)
    nfa_finalize(&frag);
    
    // 5. Debug: afficher le NFA en mode arbre
    print_nfa_tree();
    INFO("=====================================\n");
    

    print_nfa();
    INFO("=====================================\n");

    // Alternative: affichage plat
    // print_nfa();
    
    
    // 6. Matcher le NFA partout dans l'input
    INFO("Matching input: '%s'\n", input);
    match_nfa_anywhere(input);
    
    // 7. Cleanup
    nfa_free();
    RegexTreeNode_free(tree);
    
    return 0;
}