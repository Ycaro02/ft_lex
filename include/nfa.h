#ifndef NFA_IMPLEMENTATION_H
#define NFA_IMPLEMENTATION_H

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

StateSet    *create_state_set(int capacity);
void        free_state_set(StateSet *set);
NFAState    *create_state(int id, int is_final);
void        free_state(NFAState *s);
void        add_transition(NFAState *from, char c, NFAState *to);
StateSet    *add_state(StateSet *set, NFAState *s);
StateSet    *epsilon_skip(StateSet *set);

/* Debug function */
void        print_states(const char* msg, StateSet* set);

#endif /* NFA_IMPLEMENTATION_H */