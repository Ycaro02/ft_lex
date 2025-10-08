#include "../include/nfa.h"
#include "../include/log.h"


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

/* Debug function to print states in the set */
void print_states(const char* msg, StateSet* set) {
    DBG("%s:", msg);
    for(int i=0;i<set->count;i++) {
        printf("s%d", set->states[i]->id);
        print_transition(set->states[i]->transitions);
        if(i<set->count-1) printf("\n");
    }
    printf("\n");
}