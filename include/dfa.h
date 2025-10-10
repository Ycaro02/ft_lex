#ifndef DFA_IMPLEMENTATION_H
#define DFA_IMPLEMENTATION_H

#define MAX_DFA_STATES 1024
#define ALPHABET_SIZE 256

#include "bitmap.h"

/**
 * @brief Represents a DFA state
 * 
 * Each DFA state corresponds to a set of NFA states.
 */
typedef struct {
    u32     id;
    u32     is_final;
    u32     transitions[ALPHABET_SIZE];  /* transitions[c] = next state ID */
    Bitmap  nfa_states;                  /* Set of NFA states this DFA state represents */
} DFAState;

/**
 * @brief The complete DFA
 */
typedef struct {
    DFAState    states[MAX_DFA_STATES];
    u32         state_count;
    u32         start_id;
} DFA;


DFA *__get_dfa(void);

#define g_dfa (*__get_dfa())

int find_dfa_state(Bitmap *nfa_set);
u32 create_dfa_state(Bitmap *nfa_set);
void dfa_free(void);
void print_dfa(void);

#endif /* DFA_IMPLEMENTATION_H */