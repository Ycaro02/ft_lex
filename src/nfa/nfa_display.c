#include "../../include/nfa.h"
#include "../../include/log.h"

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
void print_nfa_tree_recursive(u32 state_id, char* prefix, int is_last, u32 *visited, u32* visited_count) {
    /* Check if already visited (avoid cycles) */
    for (u32 i = 0; i < *visited_count; i++) {
        if (visited[i] == state_id) return;
    }
    visited[(*visited_count)++] = state_id;
    
    NFAState *s = &g_nfa.states[state_id];
    
    /* Print current state */
    printf("%s", prefix);
    if (is_last) {
        printf("└── ");
        strcat(prefix, "    ");
    } else {
        printf("├── ");
        strcat(prefix, "│   ");
    }
    printf("State s%d%s\n", s->id, s->is_final ? " [FINAL]" : "");
    
    /* Print transitions */
    u32 trans_count = s->trans_count;
    for (u32 idx = 0; idx < trans_count; idx++) {
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
        
        /* Recursively print target state */
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
    u32 visited[2048] = {0};
    char prefix[2048] = {0};
    u32 visited_count = 0;
    
    printf("NFA Tree:\n");
    print_nfa_tree_recursive(g_nfa.start_id, prefix, 1, visited, &visited_count);
    printf("\n");
}

/* Simple flat listing */
void print_nfa(void) {
    printf("=== NFA with %d states ===\n", g_nfa.state_count);
    printf("Start: s%d\n", g_nfa.start_id);
    
    for (u32 i = 0; i < g_nfa.state_count; i++) {
        NFAState *s = &g_nfa.states[i];
        printf("s%d%s:", s->id, s->is_final ? " [FINAL]" : "");

        for (u32 j = 0; j < s->trans_count; j++) {
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
