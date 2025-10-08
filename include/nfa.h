#ifndef NFA_IMPLEMENTATION_H
#define NFA_IMPLEMENTATION_H

#include <stdbool.h>
#include "regex_tree.h"
#include "log.h"

#define DEFAULT_CAPACITY 32

typedef struct NFAState NFAState;

typedef struct Transition {
    char                c;          /* Char to match, 0 for epsilon */
    NFAState            *to;        /* Target state */
    struct Transition   *next;      /* Next transition in linked list */
} Transition;

struct NFAState {
    int         id;                 /* State ID */
    int         is_final;           /* Is final state? */
    Transition  *transitions;       /* List of transitions */
};

typedef struct StateSet {
    NFAState    **states;           /* Array of states */
    int         count;              /* Number of states */
    int         capacity;           /* Capacity of the state array */
} StateSet;

typedef struct NFAFragment {
    NFAState    *start;     /* Start state */
    StateSet    *out;       /* Out state to link */
} NFAFragment;

int *get_state_id_counter();
#define g_state_id (*get_state_id_counter())



StateSet    *create_state_set(int capacity);
void        free_state_set(StateSet *set);
NFAState    *create_state(int id, int is_final);
void        free_state(NFAState *s);
void        add_transition(NFAState *from, char c, NFAState *to);
StateSet    *add_state(StateSet *set, NFAState *s);
StateSet    *epsilon_skip(StateSet *set);


NFAFragment thompson_from_tree(RegexTreeNode *node);
char *match_nfa(NFAState* start, char* input);
void        match_nfa_anywhere(NFAState* start, char* input);
/* Debug function */
void        print_states_set(const char* msg, StateSet* set);
void        print_nfa_state(NFAState* s);
void print_nfa_fragment(const char* msg, NFAFragment* frag);

#endif /* NFA_IMPLEMENTATION_H */