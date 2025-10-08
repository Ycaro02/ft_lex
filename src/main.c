#include "../include/log.h"
#include "../include/nfa.h"

// typedef struct ClassDef {
//     char charset[1024];
//     s8   reverse_match;
// } ClassDef;

// s8 parse_class_exp(char *exp) {
//     ClassDef class_def = { .charset = "", .reverse_match = 0 };
//     int i = 0, j = 0;

//     if (exp[i] == '^') {
//         class_def.reverse_match = 1;
//         i++;
//     }

//     int exp_len = strlen(exp);

//     while (exp[i] != 0) {

//         if (i + 2 < exp_len) {
//             if (exp[i + 1] == '-' && exp[i + 2] && exp[i] < exp[i + 2]) {
//                 for (char c = exp[i]; c <= exp[i + 2]; c++) {
//                     class_def.charset[j] = c;
//                     j++;
//                 }
//                 i += 3;
//             }
//         } else {
//             class_def.charset[j] = exp[i];
//             j++;
//             i++;

//         }
//     }

//     INFO("CHARSET: %s\n", class_def.charset);
//     return (1);

// }


/* Global state ID counter */
int *get_state_id_counter() {
    static int state_id = 0;
    return (&state_id);
}

/**
 * @brief Match the input string against the regex tree
 * @param r The root of the regex tree
 * @param str The input string to match
 * @return pointer to the position in the string after matching, or NULL if no match
 */
char *match_regex(RegexTreeNode* r, char *str) {

    char *next = NULL;

    if (!r) return (str); // Empty regex matches empty string

    if (!str) return (NULL); // No string to match against


    switch (r->type) {
        case REG_CHAR:
            DBG("Matching CHAR '%c' against '%c'\n", r->c, *str);
            if (*str == r->c || (*str != '\0' && r->c == '.')) {
                DBG("Matched CHAR '%c'\n", r->c);
                next = str + 1;
            } else {
                DBG("Failed to match CHAR '%c'\n", r->c);
            }
            break;
        case REG_CLASS: {
            WARN("REG_CLASS matching not implemented yet\n");
            return (NULL);
        }
        case REG_CONCAT: {
            DBG("Matching CONCAT against '%c'\n", *str);
            next = match_regex(r->left, str);
            if (next) {
                DBG("Matched CONCAT left part\n");
                next = match_regex(r->right, next);
            }
            break;
        }
        case REG_ALT: {
            DBG("Matching ALT against '%c'\n", *str);
            next = match_regex(r->left, str);
            if (!next) {
                next = match_regex(r->right, str);
            }
            break;
        }
        default: {
            ERR("Unknown regex node type\n");
            return (NULL);
        }
    }

    switch (r->op) {
        case OP_NONE:
            return (next);
        case OP_STAR: {
            char *temp = next;
            while (temp) {
                str = temp;
                temp = match_regex(r, str);
                if (temp == str) {
                    return (str); // Prevent infinite loop
                }

            }
            return (str);
        }
        case OP_PLUS: {
            if (!next) return (NULL);
            char *temp = next;
            while (temp) {
                str = temp;
                temp = match_regex(r, str);
            }
            return (str);
        }
        case OP_OPTIONAL: {
            DBG("Matching OPTIONAL Str: '%s', r->c = '%c'\n", str, r->c);
            return (next ? next : str);
        }
        
        default:
            break;
    }

    return (next);
}

void match_regex_by_tree(RegexTreeNode *tree, char *input) {
    char *tmp = input;
    char *old = NULL;
    char *res = NULL;

    while (tmp && *tmp) {
        DBG("Attempting to match at: '%s'\n", tmp);
        
        old = tmp;
        res = match_regex(tree, tmp);
        

        if (res) {
            int len = res - old;
            if (len <= 0) {
                WARN("Zero-length match, advancing by one to avoid infinite loop\n");
                res = old + 1;
                tmp++;
                continue;
            }
            char matched[BUFF_SIZE] = {0};
            strncpy(matched, old, len);
            INFO("=====================================\n");
            INFO(PURPLE"Match Rule: %s\n"RESET, matched);
            INFO("=====================================\n");
            tmp = res;
        } else {
            DBG("No match at: '%s'\n", tmp);
            tmp++;
        }
        DBG("Continuing match at: '%s'\n", tmp);
    }

}

void print_nfa_tree_recursive(NFAState* s, char* prefix, int is_last, int *visited, int* visited_count) {
    if (!s) return;

    for (int i = 0; i < *visited_count; i++) {
        if (visited[i] == s->id) return;
    }
    visited[(*visited_count)++] = s->id;

    printf("%s", prefix);
    if (is_last) {
        printf("└── ");
        strcat(prefix, "    ");
    } else {
        printf("├── ");
        strcat(prefix, "│   ");
    }

    printf("State s%d%s\n", s->id, s->is_final ? " [FINAL]" : "");

    int count = 0;
    Transition* t = s->transitions;
    while(t) { count++; t = t->next; }

    int idx = 0;
    t = s->transitions;
    while(t) {
        idx++;
        char buf[64];
        if(t->c == 0) snprintf(buf, sizeof(buf), "ε");
        else snprintf(buf, sizeof(buf), "'%c'", t->c);

        printf("%s", prefix);
        if (idx == count) printf("└── ");
        else printf("├── ");
        printf("--%s--> s%d\n", buf, t->to->id);

        char *new_prefix = calloc(strlen(prefix) + 32, sizeof(char));
        strcpy(new_prefix, prefix);
        print_nfa_tree_recursive(t->to, new_prefix, idx == count, visited, visited_count);
        free(new_prefix);

        t = t->next;
    }
}

#define MAX_STATE_ID 1024

u64 ptr_array[1024] = {0};
u32 ptr_idx = 0;

void free_nfa_recursive(NFAState* s, int visited[]) {
    if (!s) return;


    DBG("Freeing state s%d sp:%p, tp:%p\n", s->id, s, s->transitions);

    if (visited[s->id]) return; // déjà visité
    visited[s->id] = 1;



    Transition* t = s->transitions;
    while(t) {
        free_nfa_recursive(t->to, visited);
        Transition* next = t->next;
        free(t);
        t = next;
    }

    ptr_array[ptr_idx++] = (u64)s;
}

void free_nfa(NFAState* start) {
    int visited[MAX_STATE_ID] = {0};
    free_nfa_recursive(start, visited);

    for (u32 i = 0; i < ptr_idx; i++) {
        free((void*)ptr_array[i]);
    }
}

void print_nfa_tree(NFAState* start) {
    int visited[2048] = {0};
    char prefix[2048] = {0};
    int visited_count = 0;
    printf("NFA Tree:\n");
    print_nfa_tree_recursive(start, prefix, 1, visited, &visited_count);
    printf("\n");
}

int main(int argc, char* argv[]) {
    
    set_log_level(L_INFO);

    // (void)argc;
    // parse_class_exp(argv[1]);

    if (argc < 3) {
        INFO("Usage: %s <regex> <str_to_parse>\n", argv[0]);
        return (1);
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
    
    if (tree) {
        print_regex_tree(tree);
        INFO("Parsing completed successfully!\n");
        INFO("Final position: %d/%d\n", s.pos, (int)strlen(s.str));
        INFO("=====================================\n");
        
    } else {
        ERR("Failed to parse regex!\n");
    }

    (void)input;

    // set_log_level(L_DEBUG);
    NFAFragment frag = thompson_from_tree(tree);
    
    
    // Create unique global final state
    NFAState *final = create_state(g_state_id++, 1); // mark as final

    // Connect all outputs to this final state
    for (int i = 0; i < frag.out->count; i++) {
        add_transition(frag.out->states[i], 0, final); // ε transition
        DBG("Connecting state s%d to final state s%d\n", frag.out->states[i]->id, final->id);
    }


    // print_nfa_fragment("Final NFA Fragment", &frag);
    print_nfa_tree(frag.start);
    match_nfa_anywhere(frag.start, input);

    free_nfa(frag.start);
    free_state_set(frag.out);

    // match_regex_by_tree(tree, input);
    RegexTreeNode_free(tree);

    return (0);
}

