#include "../include/log.h"
#include "../include/nfa.h"

/* Create an empty state set */
StateSet *create_state_set(int capacity) {
    StateSet *set = malloc(sizeof(StateSet));
    set->states = malloc(capacity * sizeof(NFAState*));
    set->count = 0;
    set->capacity = capacity;
    return set;
}

/* Free a state set */
void free_state_set(StateSet *set) {
    free(set->states);
    free(set);
}


/**
 * @brief Copy states from one state set to another
 * @param dest Destination state set
 * @param src Source state set
 */
void copy_state_set(StateSet *dest, StateSet *src) {
    for(int i=0;i<src->count;i++) {
        dest = add_state(dest, src->states[i]);
    }
}

/**
 * @brief Grow the state set capacity
 * @param set Pointer to the state set to grow
 * @return Pointer to the new state set with increased capacity
 */
StateSet *grow_state_set(StateSet *set) {
    StateSet *new_set = create_state_set(set->capacity * 2);
    
    copy_state_set(new_set, set);
    free_state_set(set);
    return (new_set);
}



/**
 * @brief Create a new NFA state
 * @param id State ID
 * @param is_final Is this a final state
 * @return Allocated pointer to the created state
 */
NFAState* create_state(int id, int is_final) {
    NFAState* s = malloc(sizeof(NFAState));
    s->id = id;
    s->is_final = is_final;
    s->transitions = NULL;
    return s;
}

/**
 * @brief Free an NFA state and its transitions
 * @param s Pointer to the state to free
 */
void free_state(NFAState* s) {
    Transition* t = s->transitions;
    while(t) {
        Transition* next = t->next;
        free(t);
        t = next;
    }
    free(s);
}

/**
 * @brief Add a transition from one state to another
 * @param from Source state
 * @param c Character to match (0 for epsilon)
 * @param to Target state
 */
void add_transition(NFAState* from, char c, NFAState* to) {
    Transition* t = malloc(sizeof(Transition));
    t->c = c;
    t->to = to;
    t->next = from->transitions;
    from->transitions = t;
}

/**
 * @brief Add a state to the state set if not already present
 * @param set Pointer to the state set
 * @param s State to add
 * @return Pointer to the updated state set
 */
StateSet *add_state(StateSet *set, NFAState *s) {
    for(int i = 0; i < set->count; i++) {
        if(set->states[i]==s) {
            return (set); // Already present
        }
    }
    if (set->count + 1 >= set->capacity) {
        WARN("StateSet capacity reached, growing from %d to %d", set->capacity, set->capacity * 2);
        StateSet *new_set = grow_state_set(set);
        // free(set); // Free only the struct, not the states array
        WARN("StateSet grown to capacity %d", new_set->capacity);
        set = new_set;
    }

    set->states[set->count++] = s;
    return (set);
}

/**
 * @brief Perform epsilon closure on the state set
 * @param set Pointer to the state set
 * @return Pointer to the updated state set after epsilon closure
 */
StateSet *epsilon_skip(StateSet *set) {
    for(int i=0;i<set->count;i++) {
        Transition* t = set->states[i]->transitions;
        while(t) {
            if(t->c == 0) { // epsilon
                set = add_state(set, t->to);
            }
            t = t->next;
        }
    }
    return (set);
}

/* Debug function to print a transition */
void print_transition(Transition* t) {
    if (!t) { return; }
    if(t->c==0) {
        printf(" --ε--> s%d", t->to->id);
    } else {
        printf(" --'%c'--> s%d", t->c, t->to->id);
    }
}

void print_nfa_state(NFAState* s) {
    if (!s) { return; }
    printf("State s%d%s:", s->id, s->is_final ? " [FINAL]" : "");
    Transition* t = s->transitions;
    while(t) {
        print_transition(t);
        t = t->next;
    }
    printf("\n");
}

/* Debug function to print states in the set */
void print_states_set(const char* msg, StateSet* set) {
    DBG("%s:", msg);
    for(int i=0;i<set->count;i++) {
        printf("s%d", set->states[i]->id);
        print_transition(set->states[i]->transitions);
        if (set->states[i]->is_final) {
            printf(" [FINAL]");
        }
        if(i<set->count-1) printf("\n");
    }
    printf("\n");
}

// NFAFragment thompson_from_tree(RegexTreeNode *node) {
//     if (!node) {
//         ERR("Null regex node!\n");
//         return (NFAFragment){0};
//     }

//     switch (node->type) {

//         case REG_CHAR: {
//             NFAState *s = create_state(g_state_id++, 0);
//             NFAState *e = create_state(g_state_id++, 0);
//             add_transition(s, node->c, e);


//             StateSet *out = create_state_set(DEFAULT_CAPACITY);
//             out = add_state(out, e);

//             DBG("Created CHAR fragment '%c' (%d → %d)\n", node->c, s->id, e->id);
//             return (NFAFragment){ .start = s, .out = out };
//         }

//         case REG_CONCAT: {
//             NFAFragment left = thompson_from_tree(node->left);
//             NFAFragment right = thompson_from_tree(node->right);

//             // Patch left’s dangling outputs → start of right
//             for (int i = 0; i < left.out->count; i++) {
//                 add_transition(left.out->states[i], 0, right.start); // epsilon
//             }

//             free_state_set(left.out);
//             DBG("Concatenated fragments (%d → %d)\n", left.start->id, right.start->id);
//             return (NFAFragment){ .start = left.start, .out = right.out };
//         }

//         case REG_ALT: {
//             NFAFragment left = thompson_from_tree(node->left);
//             NFAFragment right = thompson_from_tree(node->right);

//             NFAState *start = create_state(g_state_id++, 0);
//             add_transition(start, 0, left.start);
//             add_transition(start, 0, right.start);

//             // Merge outputs
//             StateSet *out = create_state_set(DEFAULT_CAPACITY + left.out->count + right.out->count);
//             for (int i = 0; i < left.out->count; i++)
//                 out = add_state(out, left.out->states[i]);
//             for (int i = 0; i < right.out->count; i++)
//                 out = add_state(out, right.out->states[i]);

//             free_state_set(left.out);
//             free_state_set(right.out);

//             DBG("Created ALT fragment (%d → {%d, %d})\n", start->id, left.start->id, right.start->id);
//             return (NFAFragment){ .start = start, .out = out };
//         }

//         default:
//             ERR("Unsupported regex node type: %d\n", node->type);
//             return (NFAFragment){0};
//     }
// }

NFAFragment thompson_from_tree(RegexTreeNode *node) {
    if (!node) {
        ERR("Null regex node!\n");
        return (NFAFragment){0};
    }

    NFAFragment frag;

    switch (node->type) {

        case REG_CHAR: {
            NFAState *s = create_state(g_state_id++, 0);
            NFAState *e = create_state(g_state_id++, 0);
            add_transition(s, node->c, e);

            StateSet *out = create_state_set(DEFAULT_CAPACITY);
            out = add_state(out, e);

            frag = (NFAFragment){ .start = s, .out = out };
            DBG("Created CHAR fragment '%c' (%d → %d)\n", node->c, s->id, e->id);
            break;
        }

        case REG_CONCAT: {
            NFAFragment left = thompson_from_tree(node->left);
            NFAFragment right = thompson_from_tree(node->right);

            for (int i = 0; i < left.out->count; i++)
                add_transition(left.out->states[i], 0, right.start);

            free_state_set(left.out);
            frag = (NFAFragment){ .start = left.start, .out = right.out };
            DBG("Concatenated fragments (%d → %d)\n", left.start->id, right.start->id);
            break;
        }

        case REG_ALT: {
            NFAFragment left = thompson_from_tree(node->left);
            NFAFragment right = thompson_from_tree(node->right);

            NFAState *start = create_state(g_state_id++, 0);
            add_transition(start, 0, left.start);
            add_transition(start, 0, right.start);

            StateSet *out = create_state_set(DEFAULT_CAPACITY + left.out->count + right.out->count);
            for (int i = 0; i < left.out->count; i++)
                out = add_state(out, left.out->states[i]);
            for (int i = 0; i < right.out->count; i++)
                out = add_state(out, right.out->states[i]);

            free_state_set(left.out);
            free_state_set(right.out);

            frag = (NFAFragment){ .start = start, .out = out };
            DBG("Created ALT fragment (%d → {%d, %d})\n", start->id, left.start->id, right.start->id);
            break;
        }

        default:
            ERR("Unsupported regex node type: %d\n", node->type);
            return (NFAFragment){0};
    }

    /* --- Gestion des opérateurs *, +, ? --- */
    switch (node->op) {
        case OP_STAR: {
            NFAState *start = create_state(g_state_id++, 0);
            NFAState *end   = create_state(g_state_id++, 0);

            add_transition(start, 0, frag.start); // epsilon to fragment start
            add_transition(start, 0, end);        // epsilon to exit
            for (int i = 0; i < frag.out->count; i++)
                add_transition(frag.out->states[i], 0, frag.start); // loop
            for (int i = 0; i < frag.out->count; i++)
                add_transition(frag.out->states[i], 0, end);         // exit

            free_state_set(frag.out);

            StateSet *out = create_state_set(DEFAULT_CAPACITY);
            out = add_state(out, end);
            frag = (NFAFragment){ .start = start, .out = out };
            DBG("Applied STAR operator (%d → %d)\n", start->id, end->id);
            break;
        }

        case OP_PLUS: {
            NFAState *end = create_state(g_state_id++, 0);

            for (int i = 0; i < frag.out->count; i++)
                add_transition(frag.out->states[i], 0, frag.start); // loop
            for (int i = 0; i < frag.out->count; i++)
                add_transition(frag.out->states[i], 0, end);         // exit

            free_state_set(frag.out);

            StateSet *out = create_state_set(DEFAULT_CAPACITY);
            out = add_state(out, end);
            frag.out = out;
            DBG("Applied PLUS operator (%d → %d)\n", frag.start->id, end->id);
            break;
        }

        case OP_OPTIONAL: {
            NFAState *start = create_state(g_state_id++, 0);
            NFAState *end   = create_state(g_state_id++, 0);

            add_transition(start, 0, frag.start);
            add_transition(start, 0, end);
            for (int i = 0; i < frag.out->count; i++)
                add_transition(frag.out->states[i], 0, end);

            free_state_set(frag.out);

            StateSet *out = create_state_set(DEFAULT_CAPACITY);
            out = add_state(out, end);
            frag = (NFAFragment){ .start = start, .out = out };
            DBG("Applied OPTIONAL operator (%d → %d)\n", start->id, end->id);
            break;
        }

        case OP_NONE:
        default:
            break;
    }

    return frag;
}


char *match_nfa(NFAState* start, char* input) {
    StateSet *current = create_state_set(DEFAULT_CAPACITY);
    current = add_state(current, start);
    current = epsilon_skip(current);
    // print_states_set("Initial states", current);

    char* ptr = input;
    char* last_accept = NULL;
    /* Check if the initial state set contains a final state (for zero-length match) */
    for (int i = 0; i < current->count; ++i) {
        if (current->states[i]->is_final) {
            last_accept = ptr; // match vide accepté à la position de départ
            DBG("Initial set contains final state: s%d (zero-length match)", current->states[i]->id);
            break;
        }
    }

    while (*ptr) {
        DBG("Scanning char '%c', remaining: '%s'", *ptr, ptr);

        StateSet *next = create_state_set(DEFAULT_CAPACITY);

        for (int i = 0; i < current->count; i++) {
            NFAState* s = current->states[i];
            Transition* t = s->transitions;
            while (t) {
                if (t->c == '.' || t->c == *ptr) {
                    next = add_state(next, t->to);
                    DBG("s%d --'%c'--> s%d", s->id, t->c == '.' ? '.' : t->c, t->to->id);
                }
                t = t->next;
            }
        }

        next = epsilon_skip(next);
        // print_states_set("Next states after epsilon closure", next);

        if (next->count == 0) {
            
            /* If there's no reachable state, stop scanning and return based on last_accept. */

            DBG("No states reachable on '%c' — stopping scan, returning based on last_accept", *ptr);
            free_state_set(next);
            break;
        }

        for (int i = 0; i < next->count; ++i) {
            if (next->states[i]->is_final) {
                last_accept = ptr + 1; // position après le caractère courant
                INFO("Found accepting state s%d at position offset %ld", next->states[i]->id, (long)(last_accept - input));
                break;
            }
        }

        free_state_set(current);
        current = next;
        ptr++;
    }

    /* If we've exited the loop because we've read all the input,
         check if the current set contains a final state. */
    if (ptr && *ptr == '\0') {
        for (int i = 0; i < current->count; ++i) {
            if (current->states[i]->is_final) {
                last_accept = ptr; // position à la fin de l'input
                INFO("Reached final state at end: s%d", current->states[i]->id);
                break;
            }
        }
    }

    free_state_set(current);

    if (last_accept) {
        char matched[1024] = {};
        int match_len = last_accept - input;

        if (!match_len) {
            WARN("Zero-length match at start");
            return (NULL); // Return the original input pointer for zero-length match
        }

        strncpy(matched, input, match_len);
        // INFO(PURPLE"Match found: matched %ld chars %s"RESET, (long)match_len, matched);
        INFO(PURPLE"Match Rule: %s\n"RESET, matched);

        return (input + match_len);
    }

    DBG("No match (no accepting prefix)");
    return (NULL);
}

void match_nfa_anywhere(NFAState* start, char* input) {

    char *p = input;
    while (*p) {
        DBG("Trying match at position: '%s'", p);
        char *match = match_nfa(start, p);
        if (match) {
            INFO("✅ Match found");
            p = match;
        } else {
            p++;
        }
    }

}