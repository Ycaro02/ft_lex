#ifndef NFA_IMPLEMENTATION_H
#define NFA_IMPLEMENTATION_H

#include <stdbool.h>
#include "regex_tree.h"
#include "log.h"


/* Initial capacity for the NFA states array */
#define DEFAULT_NFA_CAPACITY 64
/* Initial capacity for the transitions array in each state */
#define INITIAL_TRANSITIONS_CAPACITY 8

/**
 * @brief Represents a transition from one state to another
 * 
 * A transition is triggered by a specific character or epsilon (0).
 */
typedef struct {
    char    c;          // Character to match (0 for epsilon transition)
    int     to_id;      // ID of the destination state
} Transition;

/**
 * @brief Represents a state in the NFA
 * 
 * Each state has a unique ID, can be final, and contains a dynamic
 * array of transitions that automatically grows when needed.
 */
typedef struct {
    int         id;
    int         is_final;
    Transition  *trans;         // Dynamic array of transitions
    int         trans_count;    // Current number of transitions
    int         trans_capacity; // Current capacity of the transitions array
} NFAState;

/**
 * @brief Represents the complete NFA
 * 
 * Contains all states in a dynamic array and tracks the start state.
 */
typedef struct {
    NFAState    *states;        // Array of all NFA states
    int         state_count;    // Current number of states
    int         capacity;       // Current capacity of the states array
    int         start_id;       // ID of the start state
} NFA;

/**
 * @brief Represents a fragment of an NFA during Thompson construction
 * 
 * Used during the Thompson construction algorithm to build the NFA
 * incrementally. Each fragment has a start state and multiple output states.
 */
typedef struct {
    int     start_id;           // ID of the fragment's start state
    int     *out_ids;           // IDs of the fragment's output states
    int     out_count;          // Number of output states
    int     out_capacity;       // Capacity of the out_ids array
} NFAFragment;

#endif /* NFA_IMPLEMENTATION_H */