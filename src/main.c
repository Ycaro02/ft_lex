#include "../include/log.h"
#include "../include/bitmap.h"
#include "../include/nfa.h"
#include "../include/dfa.h"


/* ========================================================================== */
/*                         SUBSET CONSTRUCTION                                */
/* ========================================================================== */

/**
 * @brief Compute the set of NFA states reachable on character c
 * @param from NFA state set
 * @param c Character to transition on
 * @param result Output: resulting NFA state set
 */
static void move_on_char(Bitmap *from, unsigned char c, Bitmap *result) {
    bitmap_clear(result);
    
    for (u32 i = 0; i < g_nfa.state_count; i++) {
        if (!bitmap_is_set(from, i)) continue;
        
        NFAState *s = &g_nfa.states[i];
        for (u32 j = 0; j < s->trans_count; j++) {
            /* Match on character or wildcard */
            if (s->trans[j].c == c || s->trans[j].c == NFA_DOT_CHAR) {
                bitmap_set(result, s->trans[j].to_id);
            }
        }
    }
    
    epsilon_closure(result);
}

/**
 * @brief Convert NFA to DFA using subset construction algorithm
 * 
 * This is the classic powerset construction algorithm.
 */
void nfa_to_dfa(void) {
    INFO("Converting NFA to DFA...\n");
    
    g_dfa.state_count = 0;
    
    /* Initialize with start state */
    Bitmap start_set;
    bitmap_init(&start_set, BITMAP_STATE_ARRAY_SIZE);
    bitmap_set(&start_set, g_nfa.start_id);
    epsilon_closure(&start_set);
    
    g_dfa.start_id = create_dfa_state(&start_set);
    INFO("DFA start state: %d\n", g_dfa.start_id);
    
    /* Work queue: states that need to be processed */
    u32 work_queue[MAX_DFA_STATES];
    u32 queue_size = 0;
    work_queue[queue_size++] = g_dfa.start_id;
    
    Bitmap next_set;
    bitmap_init(&next_set, BITMAP_STATE_ARRAY_SIZE);
    
    /* Process each state in the queue */
    while (queue_size > 0) {
        u32 current_id = work_queue[--queue_size];
        DFAState *current = &g_dfa.states[current_id];
        
        DBG("Processing DFA state %d\n", current_id);
        
        /* For each possible character */
        for (u32 c = 1; c < ALPHABET_SIZE; c++) {
            move_on_char(&current->nfa_states, c, &next_set);
            
            /* Skip if no states reachable */
            int has_states = 0;
            for (u32 i = 0; i < BITMAP_STATE_ARRAY_SIZE; i++) {
                if (next_set.bits[i]) {
                    has_states = 1;
                    break;
                }
            }
            if (!has_states) continue;
            
            /* Find or create DFA state for this set */
            int next_id = find_dfa_state(&next_set);
            if (next_id == -1) {
                next_id = create_dfa_state(&next_set);
                work_queue[queue_size++] = next_id;
                DBG("  Created new DFA state %d on char '%c' (0x%02x)\n", next_id, 
                    (c >= 32 && c < 127) ? c : '?', c);
            }
            
            current->transitions[c] = next_id;
        }
    }
    
    free(start_set.bits);
    free(next_set.bits);
    
    INFO("DFA construction complete: %d states (from %d NFA states)\n", 
         g_dfa.state_count, g_nfa.state_count);
}


/* ========================================================================== */
/*                   Compressed DFA Tables (like Flex does)                   */
/* ========================================================================== */

/**
 * @brief Character equivalence classes
 * 
 * Flex groups characters that always have the same transitions together.
 * Example: For regex "ab", all chars except 'a' and 'b' are equivalent.
 */
typedef struct {
    unsigned char ec[256];  /* ec[c] = equivalence class of character c */
    int num_classes;        /* Total number of equivalence classes */
} EquivClasses;


unsigned char yy_ec[256] = {};
int *yy_accept = NULL;
int *yy_nxt = NULL;
int ec_num_classes = 0;

/**
 * @brief Compute equivalence classes for compression
 */
static EquivClasses compute_equiv_classes(DFA *dfa) {
    EquivClasses ec;
    
    /* Initially, each char is its own class */
    for (int i = 0; i < 256; i++) {
        ec.ec[i] = i;
    }
    
    /* Group characters with identical transition patterns */
    int next_class = 0;
    int assigned[256] = {0};
    
    for (int c1 = 0; c1 < 256; c1++) {
        if (assigned[c1]) continue;
        
        ec.ec[c1] = next_class;
        assigned[c1] = 1;
        
        /* Find all chars with same transition pattern as c1 */
        for (int c2 = c1 + 1; c2 < 256; c2++) {
            if (assigned[c2]) continue;
            
            int same = 1;
            for (u32 s = 0; s < dfa->state_count; s++) {
                if (dfa->states[s].transitions[c1] != dfa->states[s].transitions[c2]) {
                    same = 0;
                    break;
                }
            }
            
            if (same) {
                ec.ec[c2] = next_class;
                assigned[c2] = 1;
            }
        }
        
        next_class++;
    }
    
    ec.num_classes = next_class;
    return ec;
}

static char *match_dfa_table(char *input) {
    int state = g_dfa.start_id;
    char *ptr = input;
    char *last_accept = NULL;
    if (yy_accept[state]) last_accept = ptr;
    while (*ptr) {
        int ec_val = yy_ec[(unsigned char)*ptr];  /* equivalence class */
        int next = yy_nxt[state * ec_num_classes + ec_val];
        if (next == -1) break;
        state = next;
        ptr++;
        if (yy_accept[state]) last_accept = ptr;
    }
    return last_accept;
}

static void match_dfa_anywhere_table(char *regex_str, char *input) {
    char *p = input;
    while (*p) {
        char *match = match_dfa_table(p);
        if (match) {
            int len = match - p;
            if (len == 0) { p++; continue; }
            printf("TABLE✅Match Rule: %s ", regex_str);
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

/**
 * @brief Export compressed DFA (Flex-style)
 */
void build_compress_dfa(DFA *dfa) {
    EquivClasses ec = compute_equiv_classes(dfa);
    
    memcpy(yy_ec, ec.ec, 256);
    yy_accept = malloc(sizeof(int) * dfa->state_count);
    yy_nxt = malloc(sizeof(int) * dfa->state_count * ec.num_classes);

    for (u32 i = 0; i < dfa->state_count; i++) {
        yy_accept[i] = dfa->states[i].is_final ? 1 : 0;
    }

    for (u32 s = 0; s < dfa->state_count; s++) {
        for (int c = 0; c < ec.num_classes; c++) {
            /* Find a character in this equiv class */
            int repr = -1;
            for (int i = 0; i < 256; i++) {
                if (ec.ec[i] == c) {
                    repr = i;
                    break;
                }
            }
            
            int next = (repr >= 0) ? (int)dfa->states[s].transitions[repr] : -1;
            if (next == (int)(u32)-1) next = -1;
            yy_nxt[s * ec.num_classes + c] = next;
        }
    }

    ec_num_classes = ec.num_classes;

    printf("✅ Generated compressed DFA table\n");
    printf("   Compression: %d → %d equiv classes (%.1f%% reduction)\n",
           256, ec.num_classes, 100.0 * (256 - ec.num_classes) / 256);
}

int tester(int argc, char **argv) {
    set_log_level(L_INFO);
    
    if (argc < 3) {
        INFO("Usage: %s <regex> <str_to_parse>\n", argv[0]);
        return 1;
    }
    
    char *input = argv[2];

    String s = {
        .str = argv[1],
        .pos = 0,
        .len = strlen(argv[1])
    };


    INFO("Parsing regex: '%s'\n", s.str);
    INFO("=====================================\n");
    
    RegexTreeNode *tree = parse_regex(&s);
    
    if (!tree) {
        ERR("Failed to parse regex!\n");
        return (1);
    }
    
    print_regex_tree(tree);
    INFO("Parsing completed successfully!\n");
    INFO("Final position: %d/%d\n", s.pos, (int)strlen(s.str));
    INFO("=====================================\n");
    
    nfa_init(DEFAULT_NFA_CAPACITY);
    NFAFragment frag = thompson_from_tree(tree);
    nfa_finalize(&frag);
    
    // print_nfa_tree();
    // INFO("=====================================\n");
    print_nfa();
    // INFO("=====================================\n");

    INFO("Matching input: '%s'\n", input);

    nfa_to_dfa();
    print_dfa();
    build_compress_dfa(&g_dfa);
    match_dfa_anywhere_table(argv[1], input);

    INFO("=====================================\n");

    dfa_free();
    nfa_free();
    RegexTreeNode_free(tree);
    return (0);
}


int main(int argc, char **argv) {
    return tester(argc, argv);
}