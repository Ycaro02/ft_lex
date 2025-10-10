#include "../../include/nfa.h"
#include "../../include/dfa.h"
#include "../../include/log.h"

DFA *__get_dfa(void) {
    static DFA dfa = {0};
    return (&dfa);
}

/**
 * @brief Find DFA state with matching NFA state set
 * @return DFA state ID, or -1 if not found
 */
int find_dfa_state(Bitmap *nfa_set) {
    for (u32 i = 0; i < g_dfa.state_count; i++) {
        if (bitmap_equal(&g_dfa.states[i].nfa_states, nfa_set)) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Create new DFA state from NFA state set
 * @return New DFA state ID
 */
u32 create_dfa_state(Bitmap *nfa_set) {
    if (g_dfa.state_count >= MAX_DFA_STATES) {
        ERR("DFA state limit reached!\n");
        exit(1);
    }
    
    u32 id = g_dfa.state_count++;
    DFAState *state = &g_dfa.states[id];
    
    state->id = id;
    state->is_final = 0;
    bitmap_init(&state->nfa_states, BITMAP_STATE_ARRAY_SIZE);
    bitmap_copy(&state->nfa_states, nfa_set);
    
    /* Initialize all transitions to invalid (-1) */
    for (u32 i = 0; i < ALPHABET_SIZE; i++) {
        state->transitions[i] = (u32)-1;
    }
    
    /* Check if any NFA state in this set is final */
    for (u32 i = 0; i < g_nfa.state_count; i++) {
        if (bitmap_is_set(nfa_set, i) && g_nfa.states[i].is_final) {
            state->is_final = 1;
            break;
        }
    }
    
    return id;
}

void dfa_free(void) {
    for (u32 i = 0; i < g_dfa.state_count; i++) {
        free(g_dfa.states[i].nfa_states.bits);
    }
    g_dfa.state_count = 0;
}

void print_dfa(void) {
    printf("=== DFA with %d states ===\n", g_dfa.state_count);
    printf("Start: d%d\n\n", g_dfa.start_id);
    
    for (u32 i = 0; i < g_dfa.state_count; i++) {
        DFAState *s = &g_dfa.states[i];
        printf("State d%d%s (NFA states: {", s->id, s->is_final ? " [FINAL]" : "");
        
        /* Print NFA states */
        int first = 1;
        for (u32 j = 0; j < g_nfa.state_count; j++) {
            if (bitmap_is_set(&s->nfa_states, j)) {
                if (!first) printf(", ");
                printf("%d", j);
                first = 0;
            }
        }
        printf("})\n");
        
        /* Print transitions */
        for (u32 c = 1; c < ALPHABET_SIZE; c++) {
            if (s->transitions[c] != (u32)-1) {
                printf("  --'%c'--> d%d\n", 
                       (c >= 32 && c < 127) ? c : '?', 
                       s->transitions[c]);
            }
        }
    }
    printf("\n");
}