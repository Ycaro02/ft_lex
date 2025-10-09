#ifndef NFA_IMPLEMENTATION_H
#define NFA_IMPLEMENTATION_H

#include "regex_tree.h"
#include "log.h"


/* Initial capacity for the NFA states array */
#define DEFAULT_NFA_CAPACITY 64

/* Initial capacity for the transitions array in each state */
#define INITIAL_TRANSITIONS_CAPACITY 8

/* Special character code for wildcard (.) transitions */
#define NFA_DOT_CHAR 200

/**
 * @brief Represents a transition from one state to another
 * 
 * A transition is triggered by a specific character or epsilon (0).
 */
typedef struct {
    unsigned char    c;     /* Character to match (0 for epsilon transition) */
    int     to_id;          /* ID of the destination state */
} Transition;

/**
 * @brief Represents a state in the NFA
 * 
 * Each state has a unique ID, can be final, and contains a dynamic
 * array of transitions that automatically grows when needed.
 */
typedef struct {
    u32         id;
    u32         is_final;
    Transition  *trans;         /* Dynamic array of transitions */
    u32         trans_count;    /* Current number of transitions */
    u32         trans_capacity; /* Current capacity of the transitions array */
} NFAState;

/**
 * @brief Represents the complete NFA
 * 
 * Contains all states in a dynamic array and tracks the start state.
 */
typedef struct {
    NFAState    *states;        /* Array of all NFA states */
    u32         state_count;    /* Current number of states */
    u32         capacity;       /* Current capacity of the states array */
    u32         start_id;       /* ID of the start state */
} NFA;

/**
 * @brief Represents a fragment of an NFA during Thompson construction
 * 
 * Used during the Thompson construction algorithm to build the NFA
 * incrementally. Each fragment has a start state and multiple output states.
 */
typedef struct {
    u32     start_id;           /* ID of the fragment's start state */
    u32     *out_ids;           /* IDs of the fragment's output states */
    u32     out_count;          /* Number of output states */
    u32     out_capacity;       /* Capacity of the out_ids array */
} NFAFragment;


/* nfa/nfa.c */

/* Get the static NFA instance */
NFA             *__get_nfa(void);

/* Macro to access the global NFA instance errno like macro */
#define g_nfa   (*__get_nfa())


/* NFA construction functions nfa/nfa.c */
void        nfa_init(u32 capacity);
void        nfa_free(void);
void        nfa_finalize(NFAFragment *frag);
NFAFragment thompson_from_tree(RegexTreeNode *node);


/* nfa/nfa_match.c */
void        match_nfa_anywhere(char *regex_str, char *input);
void        epsilon_closure(Bitmap *states);

/* nfa/nfa_display.c */
void        print_nfa_tree(void);
void        print_nfa(void);



#endif /* NFA_IMPLEMENTATION_H */