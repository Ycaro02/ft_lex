
#include "../../include/nfa.h"
#include "../../include/log.h"

/* Global NFA - all states are stored here */
NFA *__get_nfa(void) {
    static NFA nfa_ctx = {0};
    return (&nfa_ctx);
}


/**
 * @brief Initialize the global NFA with specified capacity
 * @param capacity Initial capacity for the states array
 */
void nfa_init(u32 capacity) {
    g_nfa.states = calloc(capacity, sizeof(NFAState));
    g_nfa.state_count = 0;
    g_nfa.capacity = capacity;
    g_nfa.start_id = -1;
}

/**
 * @brief Free all memory allocated for the NFA
 * 
 * Frees the transitions array of each state, then the states array itself.
 */
void nfa_free(void) {
    /* Free transitions array for each state */
    for (u32 i = 0; i < g_nfa.state_count; i++) {
        free(g_nfa.states[i].trans);
    }
    free(g_nfa.states);
    g_nfa.states = NULL;
    g_nfa.state_count = 0;
}

/**
 * @brief Create a new NFA state
 * @param is_final Whether this state is a final/accepting state
 * @return ID of the newly created state
 * 
 * Automatically grows the states array if capacity is reached.
 * Initializes the state with a dynamic transitions array.
 */
static u32 create_state(u32 is_final) {
    if (g_nfa.state_count >= g_nfa.capacity) {
        g_nfa.capacity *= 2;
        g_nfa.states = realloc(g_nfa.states, g_nfa.capacity * sizeof(NFAState));
    }
    
    u32 id = g_nfa.state_count++;
    g_nfa.states[id].id = id;
    g_nfa.states[id].is_final = is_final;
    g_nfa.states[id].trans = malloc(INITIAL_TRANSITIONS_CAPACITY * sizeof(Transition));
    g_nfa.states[id].trans_count = 0;
    g_nfa.states[id].trans_capacity = INITIAL_TRANSITIONS_CAPACITY;
    
    return id;
}

/**
 * @brief Add a transition from one state to another
 * @param from_id ID of the source state
 * @param c Character to match (0 for epsilon transition)
 * @param to_id ID of the destination state
 * 
 * Automatically grows the transitions array if capacity is reached.
 * This allows unlimited transitions per state.
 */
static void add_transition(u32 from_id, u8 c, u32 to_id) {
    NFAState *s = &g_nfa.states[from_id];
    
    /* Reallocate if necessary (double the capacity) */
    if (s->trans_count >= s->trans_capacity) {
        s->trans_capacity *= 2;
        s->trans = realloc(s->trans, s->trans_capacity * sizeof(Transition));
        if (!s->trans) {
            fprintf(stderr, "ERROR: Failed to realloc transitions for state %d\n", from_id);
            exit(1);
        }
    }
    
    s->trans[s->trans_count].c = c;
    s->trans[s->trans_count].to_id = to_id;
    s->trans_count++;
}

/**
 * @brief Create a new NFA fragment
 * @param start_id ID of the fragment's start state
 * @return New NFAFragment with dynamic output array
 */
static NFAFragment frag_create(u32 start_id) {
    NFAFragment f;
    f.start_id = start_id;
    f.out_ids = malloc(8 * sizeof(u32));
    f.out_count = 0;
    f.out_capacity = 8;
    return f;
}

/**
 * @brief Add an output state to a fragment
 * @param f Pointer to the fragment
 * @param state_id ID of the state to add to outputs
 * 
 * Automatically grows the output array if capacity is reached.
 */
static void frag_add_out(NFAFragment *f, u32 state_id) {
    if (f->out_count >= f->out_capacity) {
        f->out_capacity *= 2;
        f->out_ids = realloc(f->out_ids, f->out_capacity * sizeof(u32));
    }
    f->out_ids[f->out_count++] = state_id;
}

/**
 * @brief Free memory allocated for a fragment
 * @param f Pointer to the fragment to free
 */
static void frag_free(NFAFragment *f) {
    free(f->out_ids);
    f->out_ids = NULL;
    f->out_count = 0;
}

/**
 * @brief Create NFA fragment for a single character
 * @param c Character to match
 * @return Fragment with start -> c -> end
 */
static NFAFragment nfa_char(char c) {
    u32 s = create_state(0);
    u32 e = create_state(0);
    
    /* Convert '.' to special wildcard character */
    u8 transition_char = (c == '.') ? NFA_DOT_CHAR : c;
    add_transition(s, transition_char, e);
    
    NFAFragment frag = frag_create(s);
    frag_add_out(&frag, e);
    
    return frag;
}

/**
 * @brief Concatenate two NFA fragments (AB)
 * @param left Left fragment (A)
 * @param right Right fragment (B)
 * @return Combined fragment representing the concatenation
 * 
 * Connects all outputs of left to the start of right via epsilon transitions.
 */
static NFAFragment nfa_concat(NFAFragment left, NFAFragment right) {
    /* Connect all outputs of left to the start of right */
    for (u32 i = 0; i < left.out_count; i++) {
        add_transition(left.out_ids[i], 0, right.start_id);
    }
    
    NFAFragment result = frag_create(left.start_id);
    for (u32 i = 0; i < right.out_count; i++) {
        frag_add_out(&result, right.out_ids[i]);
    }
    
    frag_free(&left);
    frag_free(&right);
    
    return result;
}

/**
 * @brief Create alternation between two NFA fragments (A|B)
 * @param left Left alternative (A)
 * @param right Right alternative (B)
 * @return Fragment with new start branching to both alternatives
 * 
 * Creates a new start state with epsilon transitions to both branches.
 */
static NFAFragment nfa_alt(NFAFragment left, NFAFragment right) {
    u32 start = create_state(0);
    
    /* Epsilon transitions to both branches */
    add_transition(start, 0, left.start_id);
    add_transition(start, 0, right.start_id);
    
    NFAFragment result = frag_create(start);
    
    /* All outputs from both branches become fragment outputs */
    for (u32 i = 0; i < left.out_count; i++) {
        frag_add_out(&result, left.out_ids[i]);
    }
    for (u32 i = 0; i < right.out_count; i++) {
        frag_add_out(&result, right.out_ids[i]);
    }
    
    frag_free(&left);
    frag_free(&right);
    
    return result;
}

/**
 * @brief Apply Kleene star operator to a fragment (A*)
 * @param frag Fragment to apply star to
 * @return New fragment matching zero or more repetitions
 * 
 * Creates new start and end states. Start can skip to end (zero matches)
 * or enter fragment. Fragment outputs can loop back or exit.
 */
static NFAFragment nfa_star(NFAFragment frag) {
    u32 start = create_state(0);
    u32 end = create_state(0);
    
    /* Epsilon: start -> frag.start | start -> end */
    add_transition(start, 0, frag.start_id);
    add_transition(start, 0, end);
    
    /* All outputs: -> frag.start (loop) | -> end (exit) */
    for (u32 i = 0; i < frag.out_count; i++) {
        add_transition(frag.out_ids[i], 0, frag.start_id);
        add_transition(frag.out_ids[i], 0, end);
    }
    
    NFAFragment result = frag_create(start);
    frag_add_out(&result, end);
    
    frag_free(&frag);
    return result;
}

/**
 * @brief Apply plus operator to a fragment (A+)
 * @param frag Fragment to apply plus to
 * @return New fragment matching one or more repetitions
 * 
 * Similar to star but requires at least one match (no direct skip to end).
 */
static NFAFragment nfa_plus(NFAFragment frag) {
    u32 end = create_state(0);
    
    /* All outputs: -> frag.start (loop) | -> end (exit) */
    for (u32 i = 0; i < frag.out_count; i++) {
        add_transition(frag.out_ids[i], 0, frag.start_id);
        add_transition(frag.out_ids[i], 0, end);
    }
    
    NFAFragment result = frag_create(frag.start_id);
    frag_add_out(&result, end);
    
    frag_free(&frag);
    return result;
}

/**
 * @brief Apply optional operator to a fragment (A?)
 * @param frag Fragment to make optional
 * @return New fragment matching zero or one occurrence
 * 
 * Creates new start that can skip directly to end or enter fragment.
 */
static NFAFragment nfa_optional(NFAFragment frag) {
    u32 start = create_state(0);
    u32 end = create_state(0);
    
    add_transition(start, 0, frag.start_id);
    add_transition(start, 0, end);
    
    for (u32 i = 0; i < frag.out_count; i++) {
        add_transition(frag.out_ids[i], 0, end);
    }
    
    NFAFragment result = frag_create(start);
    frag_add_out(&result, end);
    
    frag_free(&frag);
    return result;
}

static NFAFragment nfa_class(ClassDef *class) {
    NFAFragment frag = frag_create(create_state(0));
    for (u32 i = 1; i < 128; i++) {
        if (!class->reverse_match && bitmap_is_set(&class->char_bitmap, i)) {
            u32 s = create_state(0);
            u32 e = create_state(0);
            INFO("Adding transition for char (%c)\n", i);
            add_transition(s, (char)i, e);
            add_transition(frag.start_id, 0, s);
            frag_add_out(&frag, e);
        } else if (class->reverse_match && !bitmap_is_set(&class->char_bitmap, i)) {
            u32 s = create_state(0);
            u32 e = create_state(0);
            INFO("Adding transition REVERSE for char (%c)\n", i);
            add_transition(s, (char)i, e);
            add_transition(frag.start_id, 0, s);
            frag_add_out(&frag, e);
        }
    }
    return (frag);
}

/**
 * @brief Convert a regex parse tree to NFA using Thompson's construction
 * @param node Root of the regex parse tree
 * @return NFA fragment representing the regex
 * 
 * Recursively builds the NFA bottom-up from the parse tree,
 * applying operators after building the base fragments.
 */
NFAFragment thompson_from_tree(RegexTreeNode *node) {
    if (!node) {
        fprintf(stderr, "ERROR: Null node\n");
        return frag_create(-1);
    }
    
    NFAFragment frag;
    
    switch (node->type) {
        case REG_CHAR:
            frag = nfa_char(node->c);
            break;
            
        case REG_CONCAT:
            frag = nfa_concat(thompson_from_tree(node->left), thompson_from_tree(node->right));
            break;
            
        case REG_ALT:
            frag = nfa_alt(thompson_from_tree(node->left), thompson_from_tree(node->right));
            break;
        case REG_CLASS:
            frag = nfa_class(node->class);
            break;
        default:
            fprintf(stderr, "ERROR: Unknown node type %d\n", node->type);
            return frag_create(-1);
    }
    
    /* Apply postfix operators */
    switch (node->op) {
        case OP_STAR:     frag = nfa_star(frag);     break;
        case OP_PLUS:     frag = nfa_plus(frag);     break;
        case OP_OPTIONAL: frag = nfa_optional(frag); break;
        case OP_NONE:     break;
    }
    
    return (frag);
}

/**
 * @brief Finalize the NFA by marking final states
 * @param frag The final fragment to finalize
 * 
 * Sets the global NFA start state and marks all fragment
 * output states as accepting/final states.
 */
void nfa_finalize(NFAFragment *frag) {
    g_nfa.start_id = frag->start_id;
    
    /* Mark all output states as final */
    for (u32 i = 0; i < frag->out_count; i++) {
        g_nfa.states[frag->out_ids[i]].is_final = 1;
    }
    
    frag_free(frag);
}
