#include "../../include/nfa.h"
#include "../../include/bitmap.h"

/**
 * @brief Compute epsilon closure of a state set
 * @param states Bitmap of states to compute closure for
 * 
 * Iteratively adds all states reachable via epsilon transitions
 * until no new states can be added (fixed point).
 */
void epsilon_closure(Bitmap *states) {
    int changed = 1;
    while (changed) {
        changed = 0;
        for (u32 i = 0; i < g_nfa.state_count; i++) {
            if (!bitmap_is_set(states, i)) continue;
            
            NFAState *s = &g_nfa.states[i];
            for (u32 j = 0; j < s->trans_count; j++) {
                if (s->trans[j].c == 0) {  /* epsilon transition */
                    if (!bitmap_is_set(states, s->trans[j].to_id)) {
                        // INFO("Set bit for state ID %d via epsilon from state ID %d\n", s->trans[j].to_id, s->id);
                        bitmap_set(states, s->trans[j].to_id);
                        changed = 1;
                    }
                }
            }
        }
    }
}


void bitmap_swap(Bitmap *dest, Bitmap *src) {
    void *tmp = dest->bits;

    dest->bits = src->bits;
    src->bits = tmp;
}

/**
 * @brief Match input string against the NFA
 * @param input Input string to match
 * @return Pointer to the position after the longest match, or NULL if no match
 * 
 * Uses subset construction to simulate NFA on the input string.
 * Tracks the longest accepting prefix found.
 */
static char *match_nfa(char *input) {
    Bitmap current, next;

    bitmap_init(&current, BITMAP_STATE_ARRAY_SIZE);
    bitmap_init(&next, BITMAP_STATE_ARRAY_SIZE);

    // bitmap_clear(&current);
    bitmap_set(&current, g_nfa.start_id);
    epsilon_closure(&current);
    
    char *ptr = input;
    char *last_accept = NULL;
    
    /* Check if initial state set contains a final state (empty match) */
    for (u32 i = 0; i < g_nfa.state_count; i++) {
        if (bitmap_is_set(&current, i) && g_nfa.states[i].is_final) {
            last_accept = ptr;
            break;
        }
    }
    
    while (*ptr) {
        bitmap_clear(&next);
        
        /* Transitions on current character */
        for (u32 i = 0; i < g_nfa.state_count; i++) {
            if (!bitmap_is_set(&current, i)) continue;
            
            NFAState *s = &g_nfa.states[i];
            for (u32 j = 0; j < s->trans_count; j++) {
                if (s->trans[j].c == NFA_DOT_CHAR || s->trans[j].c == *ptr) {
                    bitmap_set(&next, s->trans[j].to_id);
                }
            }
        }
        
        epsilon_closure(&next);
        
        /* If no states reachable, stop */
        int has_states = 0;
        for (u32 i = 0; i < BITMAP_STATE_ARRAY_SIZE; i++) {
            if (next.bits[i]) {
                has_states = 1;
                break;
            }
        }
        if (!has_states) break;
        
        /* Check if any state is final (accepting) */
        for (u32 i = 0; i < g_nfa.state_count; i++) {
            if (bitmap_is_set(&next, i) && g_nfa.states[i].is_final) {
                last_accept = ptr + 1;
                break;
            }
        }
        
        bitmap_swap(&current, &next);
        // current = next;
        ptr++;
    }
    
    /* Check if final state at end of input */
    if (*ptr == '\0') {
        for (u32 i = 0; i < g_nfa.state_count; i++) {
            if (bitmap_is_set(&current, i) && g_nfa.states[i].is_final) {
                last_accept = ptr;
                break;
            }
        }
    }
    
    free(current.bits);
    free(next.bits);

    return (last_accept);
}

/**
 * @brief Find all matches of the NFA pattern anywhere in the input string
 * @param input Input string to search for matches
 * 
 * Repeatedly attempts to match starting from each position in the input,
 * printing all matches found. Skips zero-length matches to avoid infinite loops.
 */
void match_nfa_anywhere(char *regex_str, char *input) {
    char *p = input;
    
    while (*p) {
        char *match = match_nfa(p);
        if (match) {
            int len = match - p;
            if (len == 0) { p++; continue;}  /* Skip zero-length matches */
            printf("✅Match Rule: %s ", regex_str);
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
